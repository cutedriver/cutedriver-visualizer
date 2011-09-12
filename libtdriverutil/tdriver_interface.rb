############################################################################
##
## Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
## All rights reserved.
## Contact: Nokia Corporation (testabilitydriver@nokia.com)
##
## This file is part of Testability Driver.
##
## If you have questions regarding the use of this file, please contact
## Nokia at testabilitydriver@nokia.com .
##
## This library is free software; you can redistribute it and/or
## modify it under the terms of the GNU Lesser General Public
## License version 2.1 as published by the Free Software Foundation
## and appearing in the file LICENSE.LGPL included in the packaging
## of this file.
##
############################################################################

############################################################################
# Initializations and requires
############################################################################

# convenience debug methods (needed until Ruby 1.9)
module Kernel
private
  def this_method
    caller[0] =~ /`([^']*)'/ and $1
  end
  def calling_method
    caller[1] =~ /`([^']*)'/ and $1
  end
end

require 'date'
require 'logger'


begin
  if /win/ =~ RUBY_PLATFORM or /mingw/ =~ RUBY_PLATFORM
    $lg = Logger.new(ENV['TEMP']+'/tdriver_visualizer_ruby.log', 'daily')
  else
    $lg = Logger.new('/tmp/tdriver_visualizer_ruby.log', 'daily')
  end
  $lg.level = Logger::DEBUG
rescue
  $lg = Logger.new(STDERR)
  $lg.level = Logger::FATAL
end

$lg.debug " ==================================== OPEN"


require 'benchmark'
require 'socket'


begin
  require 'tdriver'
  @tdriver_require_error = "" # no error
rescue LoadError => ex
  @tdriver_require_error = "LoadError: tdriver GEM\n#{ ex.message }"
rescue => ex
  @tdriver_require_error = "#{ ex.class }: #{ ex.message }\n\nBacktrace: #{ ex.backtrace.join("_") }"
end


if @tdriver_require_error.empty?
  @tdriver_gem_version = ENV['TDRIVER_VERSION']
else
  @tdriver_gem_version = 'error'
end

@tdriver_interface_rb_version = '2'
# tdriver_interface_rb_version history
# 1 : initial version
# 2 : message name changes:
#     'listener.rb emulation' -> 'visualization', 'ruby_interact.rb emulation' -> 'interaction'

@server = TCPServer.new("127.0.0.1", 0)

def puts_hello
  # stdout printout format defined by list below:
  # entries must be in that order, separated by whitespace
  # first line is to be split by whitespaces, after which string list indexes are:
  # 0: "TDriverVisualizerRubyInterface"
  # 1: "version"
  # 2: version number of script interface, to be changed with incompatible changes
  # 3: "port"
  # 4: port on localhost where script listens for client connection
  # 5: "tdriver"
  # 6: version string if tdriver required ok, "error" otherwise, with error dump starting from second line
  hellolist = [
    'TDriverVisualizerRubyInterface',
    'version', @tdriver_interface_rb_version.to_s, # protocol version, increase for incompatible changes
    'port', @server.addr[1].to_s, # port on localhost where script listens for client connection
    'tdriver', @tdriver_gem_version.to_s ] # tdriver version string, or "error" if require tdriver failed
  hellostring = hellolist.join(' ')
  STDOUT.puts hellostring

  if @tdriver_gem_version == 'error' or not @tdriver_require_error.empty?

    $lg.fatal hellostring
    $lg.fatal @tdriver_require_error
    STDOUT.puts
    return false

  else

    $lg.info hellostring
    STDOUT.flush
    return true

  end
end


############################################################################
# Utility methods
############################################################################


def create_output_file(dir, prefix, extension)
  index = 1
  filename = ""
  file = nil

  begin
    filename = File.join( @working_directory, prefix+"_#{index}."+extension )
    file = File.open(filename, "w")
  rescue => ex
    $lg.error "retry after failure to create #{filename}: #{ex.message}"
    index += 1
    raise "Failed to create file name, last attempt was #{filename}" if index > 999
    retry
  end

  return filename, file
end


############################################################################
# Ruby implementation of protocol used by C++ class TDriverRbProtocol
############################################################################


# message format:
#   Notation:
#     N: unsigned 4-byte integer in network byte order
#     N: { .. } a repating block, total length given by N
#     NA*: a text string, length in bytes given by N
#
#   Format:
#     N: sequence number
#     NA*: name string, also implies format of raw data, though there's only one format currently, QMap<QString, QStringList>
#     N: {
#       NA*: key string
#       N: raw list data {
#         NA*: list item string
#       }
#     }


def readWord(socket)
  # works like Qt QDataStream::operator>> for quint32
  buf = ""
  begin
    while buf.length != 4
      data, =  socket.recvfrom(4 - buf.length)
      raise "Connection closed" if data.length == 0
      buf += data
    end
  rescue Errno::EAGAIN, Errno::EINTR
    $lg.debug this_method + " recoverable exception"
    IO.select([socket])
    retry
  end

  word = buf.unpack('N')[0] # N=4 byte unsigned in network byte order
  return word
end


def readBytes(socket)
  # works like Qt QDataStream::readBytes, uses network byte order
  # returns raw data read

  len = readWord(socket)

  return '' if len==0 or len == 0xFFFFFFFF

  buf = ""
  begin
    while buf.length < len
      data, = socket.recvfrom(len - buf.length)
      raise "Connection closed" if data.length == 0
      buf += data
    end
  rescue Errno::EAGAIN, Errno::EINTR
    $lg.debug this_method + " recoverable exception"
    IO.select([socket])
    retry
  end

  return buf
end


def readMessage(socket)
  seqNum = readWord(socket)
  name = readBytes(socket)
  data = readBytes(socket)
  return seqNum, name, data
end


def writeRawData(socket, buf)
  len = buf.length
  while len > 0
    written = socket.write(buf)
    if written > 0 then
      len -= written
    else
      $lg.fatal this_method + " bad write, returned #{written}, fatal!"
      socket.close # causes the script to exit
    end
  end
end


def makeMsg(seqnum, name, map)
  mapdata = ""
  map.each do |key, value|
    key_s = key.to_s
    itemdata = ""
    value.to_a.each do |item|
      item_s = item.to_s
      itemdata += [item_s.length, item_s].pack('NA*')
    end
    mapdata += [ key_s.length, key_s].pack('NA*')+[itemdata.length, itemdata].pack('NA*')
  end
  data = [seqnum, name.length, name].pack('NNA*')+[mapdata.length, mapdata].pack('NA*')
  return data
end


def parseBytes(data, offset)
  len = data[offset, 4].unpack('N')[0] # N=4 byte unsigned in network byte order
  len = 0 if len == 0xFFFFFFFF

  offset += 4
  if len > 0
    bytes = data[offset, len].unpack('A*')[0]
  else
    bytes = ''
  end
  return bytes, offset+len
end


def parseFullMsg(msgdata)
  seqnum = data[0, 4].unpack('N')[0] # N=4 byte unsigned in network byte order
  name, offset = parseBytes(msgdata, offset=4)
  mapdata, offset = parseBytes(msgdata, offset)
  return seqnum, name, mapdata
end


def parseArray(data)
    offset = 0
    list = Array.new

    while offset < data.length
      item, offset = parseBytes(data, offset)
      list << item
    end

    return list
end


def parseArrayHash(data)
  offset = 0
  map = Hash.new

  while offset < data.length
    key, offset = parseBytes(data, offset)

    listdata, offset = parseBytes(data, offset)
    map[key] = parseArray(listdata)
  end

  return map
end


############################################################################
# old tdriver_rubyinteract.rb
############################################################################


module TDriver
  class << self
    alias __original_connect_sut__ connect_sut
  end
  def self.connect_sut( sut_attributes = {} )

    begin
      sut=self.__original_connect_sut__(sut_attributes)
    rescue => ex
      $lg.error "Custom connect_sut: raising __original_connect_sut__ exception: " + ex.message
      raise
    end

    begin
      $lg.info "Setting timeout to 0 for #{sut_attributes.inspect}"
      sut.instance_eval{ @test_object_factory.timeout = 0 }
    rescue => ex
      $lg.error "Custom connect_sut: ignoring @test_object_factory.timeout= exception: " + ex.message
    end

    return sut

  end
end


class Code_evaluation_sandbox

def initialize
  @ruby_interact_binding = binding
  @ruby_keywords = '
BEGIN
END
__ENCODING__
__END__
__FILE__
__LINE__
alias
and
begin
break
case
class
def
defined?
do
else
elsif
end
ensure
false
for
if
in
module
next
nil
not
or
redo
rescue
retry
return
self
super
then
true
undef
unless
until
when
while
yield
'.split #'

  @ruby_classes = '
MatchingData
SizedQueue
EOFError
Proc
RegexpError
Module
ZeroDivisionError
SignalException
Array
LoadError
Method
Continuation
Exception
Queue
StringIO
Range
IndexError
SystemCallError
UnboundMethod
Mutex
SystemExit
NoMethodError
FloatDomainError
Class
IO
ThreadError
NilClass
Data
Interrupt
Win32API
NotImplementedError
Symbol
RangeError
Dir
SLex
Rational
StopIteration
Integer
Fixnum
ConditionVariable
Thread
Numeric
TrueClass
StandardError
File
RuntimeError
NameError
Struct
RubyLex
Object
Float
Time
ScriptError
Bignum
LocalJumpError
TypeError
ThreadGroup
SecurityError
MatchData
SystemStackError
IOError
Binding
Date
String
SyntaxError
Hash
FalseClass
ArgumentError
DateTime
NoMemoryError
Regexp
'.split #'



  @split_re = /^(.*?)([A-Za-z0-9_?!]*)$/
# the '?' in (.*?) makes '*' non-greedy above

  @implicit_filter_re = /^([^a-zA-Z_]|__wrap)/
  @ok_print_classes = [ Fixnum, Bignum, Float, String, Date, DateTime, Regexp, Symbol ]
  @quotable_print_classes = [ String ]

end # Code_evaluation_sandbox#initialize


def line_completion(code, seqNum)
  code = code.to_s.rstrip
  if code.empty? then
    return { 'result_keys' => [ 'instance_variables', 'methods', 'ruby_keywords', 'ruby_classes' ],
             'instance_variables' => instance_eval('instance_variables'),
             'methods' => (methods | Kernel.methods),
             'ruby_keywords' => @ruby_keywords }
  end

    $lg.debug this_method + " code '#{code}'"

  m = @split_re.match(code)
  if m.size != 3 or not ( m[1].end_with?('.') or m[1].empty?) then
    return { 'result_keys' => [],
             'error_rubycode' => [ code ],
             'error_message' => [ 'Failed at code analyzation.'],
             'help_message' => ['The code to be completed must have a dot. Completion is done after the last dot.'] }
  end

  STDOUT.puts("\032\032START #{seqNum}\032")
  STDERR.puts("\032\032START #{seqNum}\032")
  ex = nil
  begin
    list = instance_eval { | | @ruby_interact_binding.eval("#{m[1]}methods") }
  rescue => ex
    STDERR.puts("Exception: #{ex}")
  end
  STDOUT.puts("\032\032END #{seqNum}\032")
  STDERR.puts("\032\032END #{seqNum}\032")
  STDOUT.flush
  STDERR.flush

  if ex != nil then
    return { 'result_keys' => [],
             'error_rubycode' => [ code ],
             'error_message' => [ 'Exception:', ex.to_s, ex.backtrace.join('\n') ],
             'help_message' => ['The code to be completed must not raise exceptions.'] }
  else
    method_list = Array.new
    list.find_all { |item|
      item.start_with?(m[2]) and !@implicit_filter_re.match(item)
    }.sort.each { |item|
      method_list << item.slice(m[2].size, item.size)
    }
    return { 'result_keys' => [ 'methods' ],
             'methods' => method_list }
  end
end

def line_execution(code, seqNum)
  code = code.to_s.rstrip
  STDOUT.sync = true
  STDERR.sync = true
  STDOUT.puts("\032\032START #{seqNum}\032")
  STDERR.puts("\032\032START #{seqNum}\032")
  ex = nil
  begin
    STDERR.puts "Evaluate: #{code.dump}"
    result = instance_eval { | | @ruby_interact_binding.eval(code) }
    rstring = (@ok_print_classes.member?(result.class) ? result.to_s : "<value hidden>")
    rstring = rstring[0,510] + ".." + rstring[-511,511] if rstring.size > 1024 # shorten overlong rstring
    rstirng = ':' + rstring if result.class == Symbol
    rstring = "\"" + rstring + "\"" if @quotable_print_classes.member?(result.class)
    STDERR.puts "..return: #{result.class}: #{rstring}"
  rescue => ex
    STDERR.puts("..raised: #{ex}")
  end
  STDERR.puts("\032\032END #{seqNum}\032")
  STDOUT.puts("\032\032END #{seqNum}\032")
  STDOUT.sync = false
  STDERR.sync = false

  if ex != nil then
    return { 'error_rubycode' => [ code ],
             'error_message' => [ 'Exception:', ex.to_s ],
             'help_message' => ['The evaluated code raised an exception.'] }
  else
    return { 'code' => [ code ], 'result class' => [ result.class ] }
             #'result inspect' => [ result.inspect ]
             #'result printable' => [ result.inspect.dump ]
  end
end


def invalidcmd(input)
    return { 'error_message' => [ 'bad request:', input.to_s ] }
end

end # class Code_evaluation_sandbox


############################################################################
# old listener.rb
############################################################################

class ListenerObject

  VISUALIZATION_ID = 'visualization'
  INTERACTION_ID = 'interaction'

  def initialize

    # set directory where to save xml & png
    if (/win/ =~ Config::CONFIG[ 'target_os' ] or /mingw/  =~ Config::CONFIG[ 'target_os' ]) and /darwin/io !~ Config::CONFIG[ 'target_os' ]
      # windows
      set_working_directory( File.expand_path( ENV[ 'TEMP' ] ) )
    else
      # unix/linux/other
      set_working_directory( File.expand_path( '/tmp/' ) )
    end

    instance_eval { | | $lg.debug "initial working directory: " + @working_directory }

  end

  def set_working_directory( dir )
    @working_directory = dir
  end

  def check_version
    @listener_reply['version'] = [ ENV['TDRIVER_VERSION'] ]
  end


  #DISABLE_API_TAB_PENDING_REMOVAL
  #def check_api_fixture( sut )
  #  #obsolete method, to be removed
  #  return sut.application.fixture('tasqtapiaccessor', 'version' )
  #end


  def get_behaviours_xml( sut, sut_id, object_types )

    # backwards compatibility
    begin
      _klass = TDriver::BehaviourFactory
    rescue
      _klass = MobyBase::BehaviourFactory.instance
    end

    filename_xml, file_xml = create_output_file(@working_directory, "visualizer_behaviours_#{ sut_id }", 'xml' )
    begin
      behaviour_attributes_hash = { :input_type => ['*', sut.input.to_s ], :sut_type => [ '*', sut.ui_type.upcase ], :version => [ '*', sut.ui_version ] }
      behaviours_xml = ""
      object_types.each do | object_type |
        behaviours_xml <<
          "<behaviour object_type=\"#{ object_type.to_s }\">\n" <<
            MobyUtil::XML::parse_string(
              _klass.to_xml( behaviour_attributes_hash.merge( { :object_type => ( object_type == 'sut' ? [ 'sut' ] : [ '*', object_type ] ) } ) )
            ).root.xpath('/behaviours/behaviour/object_methods/object_method').to_s <<
          "\n</behaviour>\n"
      end

      file_xml << MobyUtil::XML::parse_string( "<behaviours>\n#{ behaviours_xml }\n</behaviours>" ).to_s
    rescue
      file_xml.close
      raise
    end
    file_xml.close
    $lg.debug this_method + " wrote #{File.size?(filename_xml)/1024.0} KiB to '#{filename_xml}'"

    @listener_reply['behaviour_filename'] = [ filename_xml ]
  end


  #DISABLE_API_TAB_PENDING_REMOVAL
  #def get_fixture_xml( sut, sut_id, object_name )
  #  filename_xml, file_xml = create_output_file(@working_directory, "visualizer_class_methods_#{ sut_id }", 'xml' )
  #  begin
  #    data = sut.application.fixture('tasqtapiaccessor', 'list_class_methods', { :class => object_name } )
  #    file_xml << data
  #  ensure
  #    file_xml.close
  #  end
  #  $lg.debug this_method + " wrote #{File.size?(filename_xml)/1024.0} KiB to '#{filename_xml}'"
  #  @listener_reply['fixture_filename'] = [ filename_xml ]
  #end


  def get_signal_xml( sut, sut_id, app_name, object_id, object_type )
    $lg.debug this_method +
      " : sut.application(:name => '#{app_name}').child( :type => '#{object_type}', :id => '#{object_id}', :__index => 0).fixture('signal', 'list_signals')"
    obj = sut.application(:name => app_name.to_s).child( :type => object_type.to_s, :id => object_id.to_s, :__index => 0)

    filename_xml, file_xml = create_output_file(@working_directory, "visualizer_class_signals_#{ sut_id }", 'xml' )
    begin
      data = obj.fixture('signal', 'list_signals')      
      file_xml << data
    rescue Exception => e
      file_xml << '<tasMessage version="1.3">
      <tasInfo id="1" name="QtSignals" type="QtSignals">
        <obj env="qt" id="0" name="no signals" type="QtSignal" />
      </tasInfo>
    </tasMessage>'
    ensure
      file_xml.close
    end
    $lg.debug this_method + " wrote #{File.size?(filename_xml)/1024.0} KiB to '#{filename_xml}'"
    @listener_reply['signal_filename'] = [ filename_xml ]
  end


  def get_ui_dump( sut, sut_id, app_id = nil )
    MobyUtil::Parameter[ sut.id ][ :filter_type] = 'none'
    MobyUtil::Parameter[ sut.id ][ :use_find_object] = 'false'

    filename_xml, file_xml = create_output_file(@working_directory, "visualizer_dump_#{ sut_id }", 'xml' )
    begin
      data = sut.get_ui_dump( *[ ( { :id => app_id } unless app_id.nil? ) ].compact )
      file_xml << data
    ensure
      file_xml.close
    end

    $lg.debug this_method + " wrote #{File.size?(filename_xml)/1024.0} KiB to '#{filename_xml}'"
    @listener_reply['ui_filename'] = [ filename_xml ]
  end


  def capture_screen( sut, sut_id, app_id = nil )
    filename_png, file_png = create_output_file(@working_directory, "visualizer_dump_#{ sut_id }", 'png' )
    begin
      file_png.close
      source = 'nowhere!'
      if app_id.nil?
        sut.capture_screen( :Filename => filename_png, :Redraw => true )
        source = 'sut'
      else
        begin
          sut.application( :id => app_id ).capture_screen( "PNG", filename_png, true )
          source = 'app'
        rescue
          app_id = nil
          sut.capture_screen( :Filename => filename_png, :Redraw => true )
          source = 'sut'
        end
      end
      $lg.debug this_method + " got #{File.size?(filename_png)/1024.0} KiB to '#{filename_png}' from #{source}"

    rescue => ex
      # screen capture failed
      File.delete(filename_png) if File.exist?(filename_png )
      raise ex unless ex.message == "QtTasserver does not support the given service: screenShot"
      filename_png = ""
    end
    @listener_reply['image_filename'] = [ filename_png ]
  end


  def get_app_list( sut, sut_id )
    filename_xml, file_xml = create_output_file(@working_directory, "visualizer_applications_#{ sut_id }", 'xml' )
    begin
      output = sut.list_apps
      file_xml << output
	rescue Exception => e
      file_xml << '<tasMessage version="1.3">      
    </tasMessage>'
    ensure
      file_xml.close
    end

    @listener_reply['applications_filename'] = [ filename_xml ]
  end


  def start_recording( sut, app_id )
    MobyUtil::Recorder.start_rec( sut.application( :id => app_id ) )
  end


  def get_recorded_script( sut, app_id )
    application = sut.application( :id => app_id )
    filename_rb, file_rb = create_output_file(@working_directory, 'visualizer_rec_fragment', 'rb')
    begin
      script = MobyUtil::Recorder.print_script( sut, application )
      file_rb << script
    ensure
      file_rb.close
    end
    $lg.debug this_method + " wrote #{File.size?(filename_rb)/1024.0} KiB to '#{filename_rb}'"
    @listener_reply['record_filename'] = [ filename_rb ]
  end


  def test_script( sut, filename_rb )
    #filename_rb = File.join( @working_directory, 'visualizer_rec_fragment.rb' )
    $lg.debug this_method + "filename: '#{filename_rb}'"
    File.new( filename_rb ).each_line { | line |
      $lg.debug this_method + " line: '#{line}'"
      eval( line )
    }
  end


  def get_parameter( sut_id, para )
    para_value = MobyUtil::Parameter[ sut_id.to_sym ][ para.to_sym, nil ]
    @listener_reply['parameter'] = [ para.to_s, para_value.inspect ]
  end


  def get_all_parameters( sut_id )
    values = []
    keys = []
    paras=MobyUtil::Parameter[ sut_id.to_sym ]
    paras.each do | key, value |
      keys << key.to_s
      values << value.inspect
    end
    @listener_reply['keys'] = keys
    @listener_reply['values'] = values
  end


  def set_output_path( new_path )
    old = @working_directory
    set_working_directory( MobyUtil::FileHelper.fix_path( File.expand_path( new_path.to_s ) + "/" ) )
    $lg.debug this_method + " @working_directory changed '#{old}' -> '#{@working_directory}'"
    @listener_reply['output_path'] = [ @working_directory.to_s ]
  end

  def main_loop (conn)
    recorder = nil
    interact = Code_evaluation_sandbox.new

    while not conn.closed? do
      STDOUT.flush
      STDERR.flush
      #$lg.debug this_method + " reading message"
      seqNumIn = nameIn = dataIn = nil
      benchtime = Benchmark.measure {
        seqNumIn, nameIn, dataIn = readMessage(conn)
      }.real
      $lg.debug this_method + " GOT message after time: #{benchtime}"
      msgIn = parseArrayHash(dataIn)
      $lg.debug this_method + " MSG #{seqNumIn} #{nameIn} : #{msgIn.inspect}"

      #listener.rb was old script, which had STDIN/STDOUT interface
      if ((nameIn == VISUALIZATION_ID) and
            msgIn.key?('input') and
            not (input_array = msgIn['input']).empty?)
      then
        @listener_reply = Hash.new
        # handle commands where input_array length is 1
        break if ( input_array[0] == "quit" )

        if input_array.size >= 2
          sut_id = input_array.first.to_sym
          cmd = input_array[1].downcase.to_sym
          eval_cmd = ""
          error = false

          sut = nil
          begin
            # connect to sut, unless command does not require it
            sut = TDriver.connect_sut( :Id => sut_id ) unless [ :get_parameter, :get_all_parameters, :set_output_path, :check_version ].include?( cmd )

            begin
              if sut then
                #$lg.debug this_method + " before adjust: #{MobyUtil::Parameter[ sut.id ][:filter_type]} #{MobyUtil::Parameter[ sut.id ][:socket_read_timeout]} #{MobyUtil::Parameter[ sut.id ][:socket_write_timeout]} #{MobyUtil::Parameter[ sut.id ][:default_timeout]}"
                MobyUtil::Parameter[ sut.id ][ :filter_type] = 'none'
                #              MobyUtil::Parameter[ sut.id ][ :socket_read_timeout] = '10'
                #              MobyUtil::Parameter[ sut.id ][ :socket_write_timeout] = '10'
                #              MobyUtil::Parameter[ sut.id ][ :default_timeout] = '10'
                #$lg.debug this_method + " after adjust: #{MobyUtil::Parameter[ sut.id ][:filter_type]} #{MobyUtil::Parameter[ sut.id ][:socket_read_timeout]} #{MobyUtil::Parameter[ sut.id ][:socket_write_timeout]} #{MobyUtil::Parameter[ sut.id ][:default_timeout]}"
              end
            rescue
            end

          rescue => ex
            $lg.error this_method + " sut connect exception #{ex.class}: #{ex.message}"
            @listener_reply['exception'] = [ ex.class.to_s, ex.message.to_s, ex.backtrace.join('\n') ]
            @listener_reply['error'] = [ "Error: connection to sut (#{sut_id}) failed" ]
            error = true
          end

          if not error

            case cmd

            when :check_version
              eval_cmd = "check_version"

            when :set_output_path
              eval_cmd = "set_output_path( '#{ input_array[2] }' )"

            when :get_behaviours
              eval_cmd = "get_behaviours_xml( sut, '#{ sut_id }', [#{ input_array[2] }] )"

            when :refresh_ui
              eval_cmd = "get_ui_dump( sut, '#{ sut_id.to_s }', #{ input_array.size > 2 ? "'#{ input_array[2] }'" : "nil" } )"

            when :refresh_image
              eval_cmd = "capture_screen( sut, '#{ sut_id.to_s }', #{ input_array.size > 2 ? "'#{ input_array[2] }'" : "nil" } )"

            when :list_apps
              eval_cmd = "get_app_list( sut, '#{ sut_id }' )"

            when :disconnect
              eval_cmd = "TDriver.disconnect_sut( :Id => '#{ sut_id }' )" # this does not work with qt

            when :get_parameter
              eval_cmd = "get_parameter( sut_id , '#{ input_array[2] }' )"

            when :get_all_parameters
              eval_cmd = "get_all_parameters( sut_id )"

            when :tap
              eval_cmd = "sut.application"
              eval_cmd += "(:id=>'#{input_array[3]}')" if input_array.size > 3
              eval_cmd += ".#{input_array[2]}.tap"

            #DISABLE_API_TAB_PENDING_REMOVAL
            #when :check_fixture
            #  eval_cmd = "check_api_fixture( sut )"

            #DISABLE_API_TAB_PENDING_REMOVAL
            #when :fixture
            #  eval_cmd = "get_fixture_xml( sut, '#{ sut_id }', '#{ input_array[2] }' )"

            when :press_key
              eval_cmd = "sut.press_key( #{ input_array[2].to_sym } )"

            when :list_signals
              eval_cmd = "get_signal_xml( sut, '#{ sut_id }', '#{ input_array[2] }', '#{ input_array[3] }', '#{ input_array[4] }')"

            when :set_attribute
              attributeName = input_array[4]
              # note: with latest version of C++ code, join below is unnecessary, as input_array[5] is the entire value
              attributeValue = input_array[ 5..input_array.size ].join(' ')
              attributeType = input_array[3]
              eval_cmd = "sut.application.#{ input_array[2] }.set_attribute( '#{attributeName}', '#{attributeValue}', '#{attributeType}' )"

            when :start_record
              eval_cmd = "start_recording(sut, #{ ( input_array.size > 2 ? "'#{ input_array[2] }'" : "nil" ) })"

            when :stop_record
              eval_cmd = "get_recorded_script(sut, #{ ( input_array.size > 2 ? "'#{ input_array[2]}'" : "nil" ) })"

            when :test_record
              eval_cmd = "test_script(sut, '#{ input_array[2]}' )"

            when :start_application
              eval_cmd = "sut.run(:name=>'#{input_array[2]}', :arguments=>'#{input_array[3]}' )"

            else
              @listener_reply['exception'] = []
              @listener_reply['error'] = [ "Error: no command matched (#{cmd})" ]
              error = true

            end #case cmd

            if not error and not eval_cmd.empty?
              benchtime = Benchmark.measure {
                begin
                  #MobyUtil::Retryable.while( :times => 10, :timeout => 1, :exception => Exception ) do end
                  $lg.debug this_method + " cmd #{cmd} => eval_cmd '#{eval_cmd}'"
                  eval( eval_cmd )
                rescue => ex
                  @listener_reply['exception'] = [ ex.class.to_s, ex.message.to_s, ex.backtrace.join('\n') ]
                  @listener_reply['error'] = [ "Error: evaluating command (#{eval_cmd}) failed" ]
                  $lg.error this_method + " eval exception"
                  error = true
                end
              }.real
              $lg.debug this_method + " eval time: #{benchtime}"
            end

          else
          # error== true, because TDriver.connect_sut failed

          end # if !error

        else
          @listener_reply['exception'] = []
          @listener_reply['error'] = [ "Error: not enough parameters in command (#{cmd})" ]
          error = true

        end # if array_size >= 2

        msgOut = @listener_reply

      #ruby_interact.rb was old script, which had STDIN/STDOUT interface
      elsif ((nameIn == INTERACTION_ID) and
                msgIn.key?('command') and
                not (inputcmd = msgIn['command']).empty?)
      then
        case inputcmd[0]
          when "line_completion"
            msgOut = interact.line_completion(inputcmd[1], seqNumIn)
          when "line_execution"
            msgOut = interact.line_execution(inputcmd[1], seqNumIn)
          else
            msgOut = interact.invalidcmd(inputcmd)
        end

      elsif (nameIn == 'interact reset') then
        interact = Code_evaluation_sandbox.new
        msgOut = {}
      else
        msgOut = { 'error_message' => ['invalid request'] }

      end # if !input

      writeRawData(conn, makeMsg(seqNumIn, nameIn, msgOut))
      msgStr = msgOut.inspect.to_s
      msgStr = msgStr[0,1020] + " ..." if msgStr.size > 1024
      $lg.info this_method + " SNT #{seqNumIn} #{nameIn} : #{msgStr}"
    end # while

  end # def listener_main_loop

end

@listener = ListenerObject.new

############################################################################
# main program
############################################################################

puts_hello
benchtime = Benchmark.measure {
  begin
    $lg.debug "calling server accept"
    @accepted_connection = @server.accept
  rescue Errno::EAGAIN, Errno::EINTR #, Errno::ECONNABORTED, Errno::EPROTO
    $lg.error "recoverable accept error, retry in 1 s"
    sleep 1
    IO.select([@server])
    retry
  rescue => ex
    $lg.fatal "recoverable accept error #{ex.class}:#{ex.message}"
    exit 1
  end
}.real
$lg.debug "accept done after time: #{benchtime}"

begin
  @server.close
rescue => ex
  $lg.error "error closing server socket: #{ex.class}:#{ex.message}, ignored"
end
$lg.debug "server closed"

# send hello message with sequence number 0, and information about tdriver and ruby

# following code gets all constants starting with RUBY_ in Object class
@hello_data = Hash[Object.constants.find_all { |c| c.to_s.start_with?('RUBY_') }.map { |c| [c, [Object.const_get(c).to_s]]}]
@hello_data['tdriver'] = [ @tdriver_gem_version ]
@hello_data['version'] = [ @tdriver_interface_rb_version ]

begin
  writeRawData(@accepted_connection, makeMsg(0, "hello", @hello_data))
  $lg.debug "hello sent, calling main loop"
  @listener.main_loop(@accepted_connection)
rescue => ex
  STDERR.puts "main program caught an exception: #{ex}\n#{ex.backtrace.join('\n')}"
  $lg.fatal "main program caught an exception: #{ex}\n#{ex.backtrace.join(' <= ')}"
end
$lg.debug "EXIT"

@accepted_connection.close
@accepted_connection = nil

exit 0

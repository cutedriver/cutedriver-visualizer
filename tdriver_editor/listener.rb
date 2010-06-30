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



begin

  require 'tdriver'

rescue LoadError => ex

  # do not change the following sting
  STDOUT.puts "LoadError: TDriver GEM\n#{ ex.message }"
  exit;

rescue => ex

  STDOUT.puts "#{ ex.class }: #{ ex.message }\n\nBacktrace: #{ ex.backtrace.join("_") }"
  exit

end


# set directory where to save xml & png
if /win/ =~ Config::CONFIG[ 'target_os' ] && /darwin/io !~ Config::CONFIG[ 'target_os' ]

  # windows
  @working_directory = File.expand_path( ENV[ 'TEMP' ] )

else

  # unix/linux/other
  @working_directory = File.expand_path( '/tmp/' )

end

def check_version

  STDOUT.puts ENV['TDriver_VERSION']

end

def check_api_fixture( sut )

  sut.application.fixture('tasqtapiaccessor', 'version' )

end

def get_behaviours_xml( sut, sut_id, object_types )

  _output_filename = File.join( @working_directory, "visualizer_behaviours_#{ sut_id }" )

  # remove old file first
  File.delete( _output_filename ) if File.exist?( _output_filename )
  
  behaviour_attributes_hash = { :input_type => ['*', sut.input.to_s ], :sut_type => [ '*', sut.ui_type.upcase ], :version => [ '*', sut.ui_version ] }

  behaviours_xml = ""

  object_types.each{ | object_type | 

    behaviours_xml << 
      "<behaviour object_type=\"#{ object_type.to_s }\">\n" <<
        MobyUtil::XML::parse_string( 
          MobyBase::BehaviourFactory.instance.to_xml( behaviour_attributes_hash.merge( { :object_type => ( object_type == 'sut' ? [ 'sut' ] : [ '*', object_type ] ) } ) )
        ).root.xpath('/behaviours/behaviour/object_methods/object_method').to_s <<
      "\n</behaviour>\n"
  }

  File.open( "#{ _output_filename }.xml", 'w') { | file |

    file << MobyUtil::XML::parse_string( "<behaviours>\n#{ behaviours_xml }\n</behaviours>" ).to_s

  }


end

def get_fixture_xml( sut, sut_id, object_name )

  _output_filename = File.join( @working_directory, "visualizer_class_methods_#{ sut_id }" )

  # remove old file first
  File.delete( _output_filename ) if File.exist?( _output_filename )

  File.open( "#{ _output_filename }.xml", 'w') { | file |

    file << sut.application.fixture('tasqtapiaccessor', 'list_class_methods', { :class => object_name } )

  }
  
end

def get_signal_xml( sut, sut_id, app_name, object_id, object_type )

        _output_filename = File.join( @working_directory, "visualizer_class_signals_#{ sut_id }" )

        # remove old file first
        File.delete( _output_filename ) if File.exist?( _output_filename )

        app = sut.application(:name => app_name)

        File.open( "#{ _output_filename }.xml", 'w') { | file |

            begin
                cmd = "file << app."+object_type+"( :id => object_id ).fixture('signal', 'list_signals')"
                eval(cmd)
            rescue
                file.close
                File.delete("#{ _output_filename }.xml")
            end
        }

end
  
def takeDebugDump( sut, sut_id, app_id = nil )
    MobyUtil::Parameter[ sut.id ][ :filter_type] = 'none'
  _output_filename = File.join( @working_directory, "visualizer_dump_#{ sut_id }" )

  File.open( "#{ _output_filename }.xml", 'w') { | file |  file << sut.get_ui_dump( *[ ( { :id => app_id } unless app_id.nil? ) ].compact ) }
  
  begin

    if app_id.nil?

      sut.capture_screen( :Filename => "#{ _output_filename }.png", :Redraw => true )

    else
      begin
        sut.application( :id => app_id ).capture_screen( "PNG", "#{ _output_filename }.png", true )
      rescue
        app_id = nil
        sut.capture_screen( :Filename => "#{ _output_filename }.png", :Redraw => true )
      end
    end
  
  rescue => ex

    # screen capture failed
    File.delete("#{ _output_filename }.png") if File.exist?( "#{ _output_filename }.png" )

    # raise exception
    raise ex unless ex.message == "QtTasserver does not support the given service: screenShot"

  end

end

def get_app_list( sut, sut_id )

  _output_filename = File.join( @working_directory, "visualizer_applications_#{ sut_id }.xml" )
  
  # remove old file first
  File.delete( _output_filename ) if File.exist?( _output_filename )

  # retrieve list of running applications
  output = sut.list_apps

  # write list to file
  File.open( _output_filename, 'w') { | file | file << output }
  
end

def start_recording( sut, app_id )

  MobyUtil::Recorder.start_rec( sut.application( :id => app_id ) )

end

def get_recorded_script( sut, app_id )

  application = sut.application( :id => app_id )

  _output_filename = File.join( @working_directory, 'visualizer_rec_fragment.rb' )

  script = MobyUtil::Recorder.print_script( sut, application )

  File.open( _output_filename, 'w') { | file | file << script }

end

def test_script( sut )

  _output_filename = File.join( @working_directory, 'visualizer_rec_fragment.rb' )

  File.new( _output_filename ).each_line{ | line | eval( line ) }

end

def get_parameter( sut_id, para )

  para_value = MobyUtil::Parameter[ sut_id.to_sym ][ para.to_sym, nil ]

  STDOUT.puts para_value.to_s unless para_value.nil?

end

def set_output_path( new_path )

  @working_directory = MobyUtil::FileHelper.fix_path( File.expand_path( new_path.to_s ) + "/" ); 
  STDOUT.puts "Output path is now: #{ @working_directory }"

end

def report_done

  STDOUT.puts 'done'
  STDOUT.flush

end

report_done # once for startup, after requiring the tdriver gem

# main loop  

recorder = nil

while 

  input = STDIN.gets

  if ( !input )

    sleep 0.5

  else

    input.chop!

    break if ( input == "quit" )

    #STDIN.flush #clear input
    input_array = input.split

    if input_array.size < 2
      STDOUT.puts 'Error: not enough parameters'; STDOUT.flush
    else

    sut_id = input_array.first.to_sym
    cmd = input_array[ 1 ].downcase.to_sym
                
    eval_cmd = ""

    error = false

    begin
      # connect to sut, unless command doesn't require it
      sut = TDriver.connect_sut( :Id => sut_id ) unless [ :get_parameter, :set_output_path, :check_version ].include?( cmd )
        begin
        MobyUtil::Parameter[ sut.id ][ :filter_type] = 'none' if sut
      rescue    
      end

    rescue => ex

      STDOUT.puts "Error creating connection to #{ sut_id }\n\nException: #{ ex.message } (#{ ex.class })\nBacktrace: #{ ex.backtrace.join("\n") }"
      STDOUT.flush

      error = true

    end

    if !error

      case cmd

        when :check_version
          eval_cmd = "check_version"

        when :set_output_path
          eval_cmd = "set_output_path( '#{ input_array[ 2 ] }' )"

        when :get_behaviours
          eval_cmd = "get_behaviours_xml( sut, '#{ sut_id }', [#{  input_array[ 2 ] }] )"

        when :refresh
          eval_cmd = "takeDebugDump( sut, '#{ sut_id.to_s }', #{ input_array.size > 2 ? "'#{ input_array[ 2 ] }'" : "nil" } )"
        
        when :list_apps
          eval_cmd = "get_app_list( sut, '#{ sut_id }' )"

        when :disconnect
          eval_cmd = "TDriver.disconnect_sut( :Id => '#{ sut_id }' )" # this does not work with qt
        
        when :get_parameter
          eval_cmd = "get_parameter( sut_id , '#{ input_array[ 2 ] }' )"

        when :tap_screen
          eval_cmd = "sut.tap_screen( #{ input_array[ 2 ].to_i }, #{ input_array[ 3 ].to_i } )"

        when :tap
          eval_cmd = "sut.application#{ input_array.size > 3 ? "( :id => '#{ input_array[ 3 ] }' ).#{ input_array[ 2 ] }" : ".#{ input_array[ 2 ] }" }.tap"

        when :check_fixture
          eval_cmd = "check_api_fixture( sut )"

        when :fixture
        
#          eval_cmd = "sut.application#{ input_array[ 2 ].downcase == 'application' ? "#{ input_array[ 3 ] }" : ".#{ input_array[ 2 ] }#{ input_array[ 3 ] }" }.fixture( 'tasqtapiaccessor', 'list_class_methods', { :class => '#{ input_array[ 2 ] }' } )"

          eval_cmd = "get_fixture_xml( sut, '#{ sut_id }', '#{ input_array[ 2 ] }' )"

        when :take_screen_shot
          eval_cmd = "sut.application.#{ input_array[ 2 ] }.capture_screen( 'PNG', '#{ File.join( @working_directory, 'visualizer_dump_#{ sut_id }.png', true) }' )"

        when :press_key
          eval_cmd = "sut.press_key( #{ input_array[ 2 ].to_sym } )"

                                when :list_signals
                                        eval_cmd = "get_signal_xml( sut, '#{ sut_id }', '#{ input_array[ 2 ] }', '#{ input_array[ 3 ] }', '#{ input_array[ 4 ] }')"

        when :set_attribute

          parameter = ( input_array.size > 5 ? input_array[ 5..input_array.size ].join(" ") : parameter = input_array[ 5 ] ) 
        
          # strip single quotes added by visualizer            
          parameter = parameter [ 1..-2 ] if parameter.size > 1

          eval_cmd = "sut.application.#{ input_array[ 2 ] }.set_attribute( '#{ input_array[ 4 ] }', '#{ parameter }', '#{input_array [ 3 ] }' )"

        when :start_record
          eval_cmd = "start_recording(sut, #{ ( input_array.size > 2 ? "'#{ input_array[ 2 ] }'" : "nil" ) })"


        when :stop_record
          eval_cmd = "get_recorded_script(sut, #{ ( input_array.size > 2 ? "'#{ input_array[ 2 ]}'" : "nil" ) })"

        when :test_record
          eval_cmd = "test_script( sut )"

        else

          STDOUT.puts "Error: no command matched (#{ cmd })"
          STDOUT.flush

        end


        if not eval_cmd.empty?

          begin
            MobyUtil::Retryable.while( :times => 10, :timeout => 1, :exception => Exception ) { eval( eval_cmd ); report_done; }
          rescue => ex
            STDOUT.puts "Evaluated command: #{ eval_cmd }\n\nException: #{ ex.message } (#{ ex.class })\nBacktrace: #{ ex.backtrace.join("\n") }"
            STDOUT.flush            
          end

        end

      else

        # error connecting to sut

      end




    end

  end

end


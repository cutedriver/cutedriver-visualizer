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


STDOUT.sync = true

@interactive_context=binding()

puts "\032\032INFO: loading TDriver\032"

context_code = '
require \'tdriver\'
class TDriver
  class << self
    alias __original_connect_sut__ connect_sut
  end
  def self.connect_sut( sut_attributes = {} )
    puts "\032\032INFO: Setting timeout to 0 for #{sut_attributes[ :Id ].to_sym}\032"
    sut=self.__original_connect_sut__(sut_attributes)
    sut.instance_eval{ @_testObjectFactory.timeout = 0 }
    return sut
  end
end
'
eval(context_code, @interactive_context);

puts "\032\032INFO: TDriver loaded\032"


@ruby_keywords ="BEGIN
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
"

@ruby_classes = "
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
"


@split_re = /^(.*?)([A-Za-z0-9_?!]*)$/
# the '?' in (.*?) makes '*' non-greedy above

@implicit_filter_re = /^([^a-zA-Z_]|__wrap)/


def help
  puts "Special methods:"
  puts "help\tprint this help text"
  puts "exit\texit"
end


def line_completion count
  count = count.to_i

  if count < 1
    puts "\032\032INFO: instance_variables\032\n"
    eval("puts instance_variables", @interactive_context)
    puts "\032\032INFO: main methods\032\n", methods
    puts "\032\032INFO: ruby keywords\032\n", @ruby_keywords
    puts "\032\032INFO: ruby classes\032\n", @ruby_classes
    puts "\032\032END:\032"
    return
  end

  puts "\032\032ARGPROMPT: #{count} line(s) expected\032"
  line = ""
  while count > 0
    line = line + gets.chomp + " "
    count -= 1
  end
  line.rstrip!

  m = @split_re.match(line)

  if m.size != 3
    puts "\032\032ERROR: args not understood at all\032"
    return
  end

  if !m[1].end_with?('.') and !m[1].empty?
    puts "\032\032ERROR: can't substitute 'methods' into the line '#{m[1]}'\032"
    return
  end

  puts "\032\032INFO: methods of '#{m[1]}' starting with '#{m[2]}'\032"
  method_list = eval("#{m[1]}methods", @interactive_context)
  method_list.find_all{ |item|
    item.start_with?(m[2]) and !@implicit_filter_re.match(item)
  }.sort.each{|item|
    puts item.slice(m[2].size, item.size)
  }
  puts "\032\032END:\032"
end


def line_execution count

  count = count.to_i
  if count < 1
    puts "\032\032ERROR: invalid arg line count\032"
    return
  end

  puts "\032\032ARGPROMPT: #{count} line(s) expected\032"
  input = ""
  while count > 0
    input += gets
    count -= 1
  end

  puts "\032\032EVAL: " + input.dump + "\032"
  result = eval(input, @interactive_context)
  dump = result.inspect.to_s.dump
  if dump.length > 200
    dump = 'overlong result string hidden'
  end
  puts "\032\032END: "+ dump + "\032"
end


while true
  puts "\032\032PROMPT:\032"
  STDOUT.flush

  input = gets.chomp

  # note: this will eat whitespace inside quotes, but with current commands it shouldn't matter
  input2 = input.split
  inputcmd = input2.shift
  input2 = input2.join(' ')

  begin
    #puts inputcmd, input2
    case inputcmd
      when "exit" : exit
      when "line_completion" : line_completion input2
      when "line_execution" : line_execution input2
     else
      puts "\032\032EVAL: " + input.dump + "\032"
      result = eval(input, @interactive_context)
      puts "\032\032END: "+ result.inspect.to_s.dump + "\032"
    end

  rescue Exception => exc
    puts "\032\032ERROR: exception: " + exc.to_s + "\032"
  end
end

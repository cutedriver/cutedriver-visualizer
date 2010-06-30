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



# The tdriver_editor application example must be compiled and in PATH for this test to work 

require 'tdriver'

include TDriverVerify


Before do
  $ErrorMessage=""
  # <fixtures>            <fixture name="visualizer" plugin="visualizeraccessor" />    </fixtures>
  MobyUtil::Parameter[:sut_qt][:fixtures][:visualizer_fixture] = "visualizeraccessor"
        if /win/ =~ RUBY_PLATFORM
                @tdriver_editor_path = "C:/tdriver/visualizer/tdriver_editor.exe"
        elsif /linux/ =~ RUBY_PLATFORM
                @tdriver_editor_path = "/usr/bin/tdriver_editor"
        else
                raise 'Unsupported ruby platfrom'
        end
        #puts "***** visualizer binary: #{@tdriver_editor_path}"
end


After do
  begin
    @sut.application.close
  rescue
  end
end  


When("I run \"$application\"") do |$application|
  @os_name = ""
  @app = @sut.run( :name => $application.to_s, :arguments => "-testability")
end

When("I run visualizer binary") do
        @os_name = ""
        @app = @sut.run( :name => @tdriver_editor_path, :arguments => "-testability")
end


Then("application has custom fixture") do
  verify_equal("pingpong", 20, "fixture not working") {
    @app.fixture('visualizer_fixture', 'ping', { :data => "pingpong" }) 
  }
end  


Then("\"$application\" is running") do |$application|
  if ((@os_name == "linux") and RUBY_PLATFORM.downcase.include?("linux")) or
    ((@os_name == "windows") and RUBY_PLATFORM.downcase.include?("mswin")) or
    (@os_name == "")
    if RUBY_PLATFORM.downcase.include?("mswin")
      verify_equal($application + '.exe', 30, "Application name should match."){ 
        @app.executable_name
      }
    else
      verify_equal($application.to_s, 30, "Application name should match."){ 
        @app.executable_name 
      }
    end
  end
  
end


Then("\"$application\" is not running") do |$application|
  if ((@os_name == "linux") and RUBY_PLATFORM.downcase.include?("linux")) or
    ((@os_name == "windows") and RUBY_PLATFORM.downcase.include?("mswin")) or
    (@os_name == "")
    if RUBY_PLATFORM.downcase.include?("mswin")
      verify_false(30, 'application should not be running') {
        @sut.application.name == $application + '.exe'
      }
    else
      verify_false(30, 'application should not be running') {
        @sut.application.name == $application 
      }
    end
  end
end


When("I close the application") do  
  @sut.application.close
end


Given("visualizer application is ready") do
  @sut = TDriver.sut(:Id=>'sut_qt')
  @os_name = ""
        @app = @sut.run( :name => @tdriver_editor_path, :arguments => "-testability")
  if ((@os_name == "linux") and RUBY_PLATFORM.downcase.include?("linux")) or
    ((@os_name == "windows") and RUBY_PLATFORM.downcase.include?("mswin")) or
    (@os_name == "")
    if RUBY_PLATFORM.downcase.include?("mswin")
      #verify_equal($application + '.exe', 30, "Application name should match."){ @app.executable_name }
    else
      #verify_equal($application.to_s, 30, "Application name should match."){ @app.executable_name }
    end
  end
end

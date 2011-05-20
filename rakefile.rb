############################################################################
## 
## Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). 
## All rights reserved. 
## Contact: Nokia Corporation (testabilitydriver@nokia.com) 
## 
## This file is part of Testability Driver Qt Agent
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


desc "Task for testing visualizer"
task :test do
   begin
     Dir.chdir("test")
     result=system( "cucumber features -f TDriverReport::CucumberReporter" )
     raise "Feature tests failed" if (result != true) or ($? != 0) 
   ensure    
     Dir.chdir("..")	 
	if ENV['CC_BUILD_ARTIFACTS']    
    #Copy results to build artifacts
	  Dir.foreach("#{Dir.pwd}/test/tdriver_reports") do |entry|
	    if entry.include?('test_run')
	      FileUtils.cp_r "#{Dir.pwd}/test/tdriver_reports/#{entry}", "#{ENV['CC_BUILD_ARTIFACTS']}/#{entry}"
          FileUtils::remove_entry_secure("#{Dir.pwd}/test/tdriver_reports/#{entry}", :force => true)
	    end
	  end
    end
   end
   exit(0)
   
end

desc "Task for cruise control"
task :cruise => ['build_visualizer', 'test'] do
	exit(0)
end

desc "Task for building the example QT application(s)"
task :build_visualizer do

  puts "#########################################################"
  puts "### Building visualizer                               ####"
  puts "#########################################################"

  make = "make"
  sudo = ""	

  if /win/ =~ RUBY_PLATFORM || /mingw32/ =~ RUBY_PLATFORM
    make = "mingw32-make"
  else
    sudo = "sudo -S "
  end
  cmd = sudo + " #{make} uninstall"
  system(cmd)

  cmd = "#{make} distclean"
  system(cmd)
  
  cmd = "qmake CONFIG+=release"
  failure = system(cmd)
  raise "qmake failed" if (failure != true) or ($? != 0) 
    
  cmd = "#{make}"
  failure = system(cmd)
  raise "make release failed" if (failure != true) or ($? != 0) 
    
  cmd = sudo + "#{make} install"
  failure = system(cmd)
  raise "make install failed" if (failure != true) or ($? != 0) 
  puts "visualizer built"
end





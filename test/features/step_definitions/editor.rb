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



# The tdriver_visualizer application example must be compiled and in PATH for this test to work 
require 'tdriver'

include TDriverVerify

Before do
  $ErrorMessage=""
end

When /^I press "([^\"]*)"$/ do |arg1|
  pending
  #@app.TDriverCodeTextEdit.type_text("Hello, World!");
end

Then /^editor stuff is ok$/ do

  verify {
    @app.QDockWidget(:name => 'editor')
  }
  verify_equal("true", 5, "Goo") {
    @app.QMenuBar(:name => 'main menubar').attribute('visible')
  }
  verify_equal("true", 5, "Foo") {
    @app.QAction(:name => 'main file').attribute('visible')
  }
  verify_equal("true", 5, "Foo") {
    @app.QAction(:name => 'main file').attribute('visible')
  }
  verify { @app.QMenuBar(:name => 'editor').QAction( :name => 'editor file' ) }
end


When /^I create new document$/ do
  @app.QMenuBar(:name => arg1.to_s).QAction( :name => arg2.to_s ).trigger
  @app.MainWindow.QMenu(:name => arg1.to_s).QAction( :name => arg2.to_s ).trigger
end


Then /^editor is visible$/ do
  verify(5, "Editor visibility") {
    @app.TDriverCodeTextEdit(:visible => 'true')
  }
end


When /^I trigger menubar "([^\"]*)" action "([^\"]*)"$/ do |arg1, arg2|
  @app.QMenuBar(:name => arg1.to_s).QAction( :name => arg2.to_s ).trigger
end


Then /^menu "([^\"]*)" is visible$/ do |arg1|
  verify { @app.MainWindow.QMenu(:name => arg1.to_s).QAction( :name => 'editor new' ) }
end


When /^I trigger menu "([^\"]*)" action "([^\"]*)"$/ do |arg1, arg2|
  @app.MainWindow.QMenu(:name => arg1.to_s).QAction( :name => arg2.to_s ).trigger
end


Then /^file "([^\"]*)" is shown in editor$/ do |arg1|
  verify { @app.TDriverCodeTextEdit( :name => "editor file #{arg1}") }
end

Then /^temporary file "([^\"]*)" is shown in editor$/ do |arg1|
  if RUBY_PLATFORM.downcase.include?("mswin") 
    filename = "C:/temp/#{arg1}"
  else
    filename = "/tmp/#{arg1}"
  end
  
  verify { @app.TDriverCodeTextEdit( :name => "editor file #{arg1}") }
end

When /^I enter text "([^\"]*)"$/ do |arg1|
  @app.TDriverCodeTextEdit.type_text(arg1.to_s)  
end


Then /^editor contains "([^\"]*)"$/ do |arg1|
  verify { @app.TDriverCodeTextEdit(:plainText => arg1.to_s) }
end


When /^I save temporary file "([^\"]*)"$/ do |arg1|
  if RUBY_PLATFORM.downcase.include?("mswin") 
    filename = "C:/temp/#{arg1}"
  else
    filename = "/tmp/#{arg1}"
  end

  @app.TDriverTabbedEditor.fixture('visualizer_fixture', 'ping', { :data => filename })   
  @app.TDriverTabbedEditor.fixture('visualizer_fixture', 'editor_save', { :filename => filename })   
end


When /^I load temporary file "([^\"]*)"$/ do |arg1|
  if RUBY_PLATFORM.downcase.include?("mswin") 
    filename = "C:/temp/#{arg1}"
  else
    filename = "/tmp/#{arg1}"
  end

  @app.TDriverTabbedEditor.fixture('visualizer_fixture', 'editor_load', { :filename => filename }) 
end


Then /^temporary file "([^\"]*)" exists in filesystem$/ do |arg1|
  if RUBY_PLATFORM.downcase.include?("mswin") 
    filename = "C:/temp/#{arg1}"
  else
    filename = "/tmp/#{arg1}"
  end
  verify { FileTest.exists?(filename) }
end

When /^I close the document$/ do
  @app.QMenuBar(:name => arg1.to_s).QAction( :name => arg2.to_s ).trigger
  @app.MainWindow.QMenu(:name => arg1.to_s).QAction( :name => arg2.to_s ).trigger

end

Then /^editor has no files open$/ do
  verify { @app.TDriverTabbedEditor.QStackedWidget(:count => '0') }
end
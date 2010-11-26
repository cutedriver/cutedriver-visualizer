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


#When("I check menuitem \"\"/\"\"/\"\"") do |$menu,$submenu,$item|
#  if @app.QAction(:iconText => 'View') .
#end


When("I set action \"$action\" check state to \"$state\"") do |$action,$state|
  @app.QAction(:iconText => $action) .set_attribute('checked', $state)
end


Then("dockwidget \"$name\" visibility is \"$visibility\"") do |$name,$visibility|
  verify_equal($visibility, 5, "Dockwidget $name visibility should have been $visibility.") {
    @app.QDockWidget(:name=> $name.to_s).attribute('visible')
  }
end

Then("dockwidget title \"$windowtitle\" visibility is \"$visibility\"") do |$windowtitle,$visibility|
  verify_equal($visibility, 5, "Dockwidget $windowtitle visibility should have been $visibility.") {
    @app.QDockWidget(:windowTitle => $windowtitle.to_s).attribute('visible')
  }
end

When /^I make dockwidget "([^\"]*)" visible$/ do |arg1|
  if not @app.test_object_exists?( "QDockWidget", {:windowTitle => arg1.to_s, :__timeout => 0 })
    @app.QMenuBar(:name => "main menubar").QAction( :name => "main view").trigger
    @app.MainWindow.QMenu( :name => 'main view' ).QAction(:text => 'Docks and toolbars').trigger
    @app.MainWindow.QMenu( :title => 'Docks and toolbars').QAction(:text => arg1.to_s).trigger
  end
end

When /^I make dockwidget "([^\"]*)" hidden$/ do |arg1|
  if @app.test_object_exists?( "QDockWidget", {:windowTitle => arg1.to_s, :__timeout => 0 })
    @app.QMenuBar(:name => "main menubar").QAction( :name => "main view").trigger
    @app.MainWindow.QMenu( :name => 'main view' ).QAction(:text => 'Docks and toolbars').trigger
    @app.MainWindow.QMenu(:title => 'Docks and toolbars').QAction(:text => arg1.to_s).trigger
  end
end

When("I trigger action \"$icontext\"") do |$icontext|
  @application.QAction(:iconText => $icontext.to_s).trigger
end


Then("action \"$icontext\" visibility is \"$visibility\"") do |$icontext,$visibility|
  verify_equal($visibility, 5, "Widget $icontext visibility should have been $visibility.") {
    @app.QAction(:iconText => $icontext.to_s).attribute('visible')
  }
end

When("I open menu \"$titletext\"") do |$titletext|
  @application.QMenu(:title => $titletext.to_s).activate
end

Then("menu \"$titletext\" visibility is \"$visibility\"") do |$titletext,$visibility|
  verify_equal($visibility, 5, "Widget $titletext visibility should have been $visibility.") {
    @app.QMenu(:title => $titletext.to_s).attribute('visible')
  }
end

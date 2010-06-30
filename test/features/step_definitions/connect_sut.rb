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



# The calculator application example must be compiled and in PATH for this example to work 
# sut_qt1, sut_qt2, sut_qt3 and sut_qt4 suts must be defined in tdriver_parameters.xml
# /tdriver/Tutorials/QT/application_examples/calculator/release
require 'tdriver'

include TDriverVerify

Before do
  $ErrorMessage=""
  $Clear_All_Button_Text=""
  $Clear_All_Button_Text1=""
  $Clear_All_Button_Text2=""

  MobyUtil::Parameter.instance.load_parameters_xml(Dir.getwd+'/features/step_definitions/my_suts.xml')
end
  
Given("the sut \"$sut_id\" exists in tdriver parameters") do |$sut_id|
  @sut = TDriver.sut(:Id=>$sut_id.to_s)
end


When("I connect sut \"$sut_id\"") do |$sut_id|
  @sut = TDriver.connect_sut(:Id=>$sut_id.to_s)  
end


Then("the sut \"$sut_id\" is connected") do |$sut_id|
   verify(30){
    $Clear_All_Button_Text = @calc.Button(:text => 'Clear All').attribute("text")
    $Clear_All_Button_Text.should == "Clear All"
  } 
end


When("I can not connect to sut \"$sut_id\"") do |$sut_id|
  begin
    @sut= TDriver.connect_sut(:Id=>$sut_id.to_s)
    rescue RuntimeError, ArgumentError, NameError => $ErrorMessage    
    $ErrorMessage.should_not == ""
  end    
end


Then("the sut \"$sut_id\" should not be connected") do |$sut_id|
  begin
    $Clear_All_Button_Text = @calc.Button(:text => 'Clear All').attribute("text")
    $Clear_All_Button_Text.should == ""
    @calc.close
    rescue RuntimeError, ArgumentError, NameError => $ErrorMessage    
    $ErrorMessage.should_not == ""
  end
end

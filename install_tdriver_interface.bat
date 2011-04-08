mkdir C:\tdriver\visualizer
ruby -cw libtdriverutil\tdriver_interface.rb && copy /y libtdriverutil\tdriver_interface.rb C:\tdriver\visualizer\tdriver_interface.rb

mkdir C:\tdriver\visualizer\templates
copy libtdrivereditor\templates\*.* C:\tdriver\visualizer\templates

mkdir C:\tdriver\visualizer\completions
copy libtdrivereditor\completions\*.* C:\tdriver\visualizer\completions

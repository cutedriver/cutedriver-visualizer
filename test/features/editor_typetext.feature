Feature: Basic text typing in Visualizer Editor
	As a TDriver script developer,
	I want to manipulate (create, edit, save, open) files using editor.

@pass
	Scenario: Edit new document
		Given visualizer application is ready
		When I make dockwidget "Code Editor" hidden
		And I make dockwidget "Code Editor" visible
		Then dockwidget "editor" visibility is "true"
		And dockwidget title "Code Editor" visibility is "true"
		And editor stuff is ok
		When I trigger menubar "editor" action "editor file"
		And I trigger menu "editor file" action "editor new"
		Then file "" is shown in editor
		When I enter text "Hello"
		Then editor contains "Hello"
		When I trigger menubar "editor" action "editor edit"
		And I trigger menu "editor edit" action "editor undo"
		Then editor contains ""

@pass
@fixture
	Scenario: Edit new document, save, close, load
		Given visualizer application is ready
		Then application has custom fixture
		When I make dockwidget "Code Editor" hidden
		And I make dockwidget "Code Editor" visible
		Then dockwidget "editor" visibility is "true"
		And dockwidget title "Code Editor" visibility is "true"
		And editor stuff is ok
		When I trigger menubar "editor" action "editor file"
		And I trigger menu "editor file" action "editor new"
		Then file "" is shown in editor
		When I enter text "Hello"
		Then editor contains "Hello"
		When I save temporary file "tmp_feature.txt"
		Then temporary file "tmp_feature.txt" is shown in editor
		And temporary file "tmp_feature.txt" exists in filesystem
		When I trigger menubar "editor" action "editor file"
		And I trigger menu "editor file" action "editor close"
		Then editor has no files open
		When I load temporary file "tmp_feature.txt"
		Then temporary file "tmp_feature.txt" is shown in editor
		And editor contains "Hello"

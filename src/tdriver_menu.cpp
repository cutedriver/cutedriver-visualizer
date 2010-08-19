/***************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (testabilitydriver@nokia.com)
**
** This file is part of Testability Driver.
**
** If you have questions regarding the use of this file, please contact
** Nokia at testabilitydriver@nokia.com .
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/



#include "tdriver_main_window.h"

void MainWindow::createTopMenuBar() {

    menubar = new QMenuBar();
    menubar->setObjectName( "main menubar" );

    createFileMenu();
    createViewMenu();
    createSearchMenu();
    createDevicesMenu();
    createApplicationsMenu();
    createRecordMenu();
    createHelpMenu();
}


void MainWindow::createFileMenu() {

    // menu

    fileMenu = new QMenu( tr( " &File " ), this );
    fileMenu->setObjectName("main file");

    // parse tdriver parameters xml

    parseSUT = fileMenu->addAction( "&Parse TDriver parameters xml file..." );
    parseSUT->setObjectName("main parsesut");
    parseSUT->setShortcuts(QList<QKeySequence>() <<
                           QKeySequence(tr("Ctrl+M, P")) <<
                           QKeySequence(tr("Ctrl+M, Ctrl+P")));
    parseSUT->setDisabled( true );
    fileMenu->addSeparator();

    // Load state xml file

    loadXmlAction = new QAction( this );
    loadXmlAction->setObjectName("main loadxml");
    loadXmlAction->setText( "&Load state XML file" );
    loadXmlAction->setShortcuts(QList<QKeySequence>() <<
                                QKeySequence(tr("Ctrl+M, L")) <<
                                QKeySequence(tr("Ctrl+M, Ctrl+L")));
    fileMenu->addAction( loadXmlAction );

    // save state xml

    saveStateAction = new QAction( this );
    saveStateAction->setObjectName("main savestate");
    saveStateAction->setText( "&Save current state to folder..." );
    saveStateAction->setShortcuts(QList<QKeySequence>() <<
                                  QKeySequence(tr("Ctrl+M, S")) <<
                                  QKeySequence(tr("Ctrl+M, Ctrl+S")));
    fileMenu->addAction( saveStateAction );

    // font

    fileMenu->addSeparator();
    fontAction = new QAction( this );
    fontAction->setObjectName("main font");
    fontAction->setText( "Select default f&ont..." );
    //fontAction->setShortcut( QKeySequence( Qt::ControlModifier + Qt::Key_T ) );
    fileMenu->addAction( fontAction );
    fileMenu->addSeparator();

    // refresh

    refreshAction = new QAction( this );
    refreshAction->setObjectName("main refresh");
    refreshAction->setText( "&Refresh" );
    refreshAction->setShortcut(QKeySequence(tr("Ctrl+R")));
    // this is F5 in some platforms, Ctrl+R in some: QKeySequence(QKeySequence::Refresh);
    refreshAction->setDisabled( true );
    fileMenu->addAction( refreshAction );

    // tap and auto-refresh on Image View click
    //note:  action constructed in MainWindow::createImageViewDockWidget()

    // disconnect

    disconnectCurrentSUT = new QAction( this );
    disconnectCurrentSUT->setObjectName("main disconnectsut");
    disconnectCurrentSUT->setText( "Dis&connect SUT" );
    disconnectCurrentSUT->setShortcuts(QList<QKeySequence>() <<
                                       QKeySequence(tr("Ctrl+M, C")) <<
                                       QKeySequence(tr("Ctrl+M, Ctrl+C")));
    disconnectCurrentSUT->setDisabled( true );
    fileMenu->addAction( disconnectCurrentSUT );
    fileMenu->addSeparator();

    // exit

    exitAction = fileMenu->addAction( "E&xit" );
    exitAction->setObjectName("main exit");
    exitAction->setShortcut( QKeySequence( tr("Alt+X")));

    menubar->addMenu( fileMenu );
    menubar->actions().last()->setObjectName("main file");


    // signals

    connect( parseSUT, SIGNAL( triggered() ), this, SLOT( getParameterXML() ) );
    connect( loadXmlAction, SIGNAL( triggered() ), this, SLOT( loadFileData() ) );
    connect( saveStateAction, SIGNAL( triggered() ), this, SLOT( saveStateAsArchive() ) );
    connect( fontAction, SIGNAL( triggered() ), this, SLOT( openFontDialog() ) );
    connect( refreshAction, SIGNAL( triggered() ), this, SLOT( forceRefreshData() ) );
    connect( disconnectCurrentSUT, SIGNAL( triggered() ), this, SLOT( disconnectSUT() ) );
    connect( exitAction, SIGNAL( triggered() ), this, SLOT( close() ) );

}

void MainWindow::createSearchMenu() {

    searchMenu = new QMenu( " &Search ", this );
    searchMenu->setObjectName("main search");

    // dockable widget menu: clipboard

    findAction = new QAction( this );
    findAction->setObjectName("main find");
    findAction->setText( "&Find" );
    findAction->setShortcut( QKeySequence(tr("Ctrl+F")));
    searchMenu->addAction( findAction );

    menubar->addMenu( searchMenu );
    menubar->actions().last()->setObjectName("main search");

    connect( findAction, SIGNAL( triggered() ), this, SLOT( showFindDialog() ) );
}

void MainWindow::createViewMenu() {

    // menu

    viewMenu = new QMenu( " &View ", this );
    viewMenu->setObjectName("main view");
    // dockable widget menu

    viewMenuDockWidgets = new QMenu( "&Dockable widgets", viewMenu );
    viewMenuDockWidgets->setObjectName("main view dockwidgets");

    // dockable widget menu: clipboard

    viewClipboard = new QAction( this );
    viewClipboard->setObjectName("main toggle clipboard");
    viewClipboard->setText( "&Clipboard" );
    viewClipboard->setCheckable( true );
    viewMenuDockWidgets->addAction( viewClipboard );

    // dockable widget menu: view images

    viewImage = new QAction( this );
    viewImage->setObjectName("main toggle imageview");
    viewImage->setText( tr( "&Image" ) );
    viewImage->setCheckable( true );
    viewMenuDockWidgets->addAction( viewImage );

    // dockable widget menu: properties view

    viewProperties = new QAction( this );
    viewProperties->setObjectName("main toggle properties");
    viewProperties->setText( tr( "&Properties" ) );
    viewProperties->setEnabled( false );
    viewProperties->setCheckable( true );
    viewMenuDockWidgets->addAction( viewProperties );
    viewProperties->setVisible( false );

    // dockable widget menu: shortcuts

    viewShortcuts = new QAction( this );
    viewShortcuts->setObjectName("main toggle shortcuts");
    viewShortcuts->setText( tr( "&Shortcuts" ) );
    viewShortcuts->setCheckable( true );
    viewMenuDockWidgets->addAction( viewShortcuts );

    // dockable widget menu: editor

    viewEditor = new QAction( this );
    viewEditor->setObjectName("main toggle editor");
    viewEditor->setText( tr( "Code &Editor" ) );
    viewEditor->setCheckable( true );
    viewMenuDockWidgets->addAction( viewEditor );


    // dockable widget menu: (s60) navigation buttons

    viewButtons = new QAction( this );
    viewButtons->setObjectName("main toggle navigationbuttons");
    viewButtons->setText( tr( "&Navigation buttons" ) );
    viewButtons->setCheckable( true );
    viewMenuDockWidgets->addAction( viewButtons );

    viewMenu->addMenu( viewMenuDockWidgets );
    viewMenu->actions().last()->setObjectName("main view widgets");

    // object tree menu:

    viewMenuObjectTree = new QMenu( "&Object tree", viewMenu );
    viewMenuObjectTree->setObjectName("main view tree");

    // object tree menu: expand all

    viewExpandAll = new QAction( this );
    viewExpandAll->setObjectName("main tree expand");
    viewExpandAll->setText( "&Expand all" );
    viewExpandAll->setShortcut( QKeySequence( Qt::ControlModifier + Qt::Key_Right ) );
    viewExpandAll->setCheckable( false );
    viewMenuObjectTree->addAction( viewExpandAll );

    // object tree menu: collapse all

    viewCollapseAll = new QAction( this );
    viewCollapseAll->setObjectName("main tree collapse");
    viewCollapseAll->setText( "&Collapse all" );
    viewCollapseAll->setShortcut( QKeySequence( Qt::ControlModifier + Qt::Key_Left ) );
    viewCollapseAll->setCheckable( false );
    viewMenuObjectTree->addAction( viewCollapseAll );

    viewMenu->addMenu( viewMenuObjectTree );
    viewMenu->actions().last()->setObjectName("main view tree");

    viewMenu->addSeparator();

    // Show XML

    showXmlAction = new QAction( this );
    showXmlAction->setObjectName("main showxml");
    showXmlAction->setText( tr( "&Show source XML" ) );
    showXmlAction->setShortcut( QKeySequence( Qt::ControlModifier + Qt::Key_U ) );
    viewMenu->addAction( showXmlAction );

    menubar->addMenu( viewMenu );
    menubar->actions().last()->setObjectName("main view");

    // signals

    connect( viewClipboard, SIGNAL( triggered() ), this, SLOT( changeClipboardVisibility() ) );
    connect( viewImage, SIGNAL( triggered() ), this, SLOT( changeImagesVisibility() ) );
    connect( viewProperties, SIGNAL( triggered() ), this, SLOT( changePropertiesVisibility() ) );
    connect( viewShortcuts, SIGNAL( triggered() ), this, SLOT( changeShortcutsVisibility() ) );
    connect( viewEditor, SIGNAL( triggered() ), this, SLOT( changeEditorVisibility() ) );
    connect( viewButtons, SIGNAL( triggered() ), this, SLOT( changeKeyboardCommandsVisibility() ) );

    connect( viewExpandAll, SIGNAL( triggered() ), this, SLOT( objectTreeExpandAll() ) );
    connect( viewCollapseAll, SIGNAL( triggered() ), this, SLOT( objectTreeCollapseAll() ) );

    connect( showXmlAction, SIGNAL( triggered() ), this, SLOT( showXMLDialog() ) );

}

void MainWindow::createDevicesMenu() {

    // menu

    deviceMenu = new QMenu( tr( " &Devices " ), this );
    deviceMenu->setObjectName("main device");
    deviceMenu->setDisabled( true );
    menubar->addMenu( deviceMenu );
    menubar->actions().last()->setObjectName("main device");

}

void MainWindow::createApplicationsMenu() {

    // menu

    appsMenu = new QMenu( tr( " &Applications " ), this );
    appsMenu->setObjectName("main applications");
    appsMenu->setDisabled( true );
    menubar->addMenu( appsMenu );
    menubar->actions().last()->setObjectName("main applications");

}

void MainWindow::createRecordMenu() {

    // menu

    recordMenu = new QMenu( tr( " &Record " ), this );
    recordMenu->setObjectName("main record");
    recordMenu->setDisabled( true );

    // open record dialog

    recordAction = new QAction( this );
    recordAction->setObjectName("main record");
    recordAction->setText( tr( "&Open Recorder Dialog" ) );
    recordAction->setShortcut( tr( "F6" ) );

    recordMenu->addAction( recordAction );
    menubar->addMenu( recordMenu );
    menubar->actions().last()->setObjectName("main record");

    // signals

    connect( recordAction, SIGNAL( triggered() ), this, SLOT( openRecordWindow() ) );

}



void MainWindow::createHelpMenu()
{
    // menu
    helpMenu = new QMenu( tr( " &Help " ), this );
    helpMenu->setObjectName("main help");

    // TDriver help
    visualizerAssistant = new QAction( this );
    visualizerAssistant->setObjectName("main help assistant");
    visualizerAssistant->setText( tr( "TDriver &Help" ) );
    // Remove the next line if F1 presses are to be handled in the custom event handler ( for context sensitivity ).
    visualizerAssistant->setShortcut( tr( "F1" ) );
    helpMenu->addAction( visualizerAssistant );

    // Visualizer Help
    visualizerHelp = new QAction( this );
    visualizerHelp->setObjectName("main help visualizer");
    visualizerHelp->setText( tr( "&Visualizer Help" ) );
    helpMenu->addAction( visualizerHelp );
    helpMenu->addSeparator();

    // About TDriver
    aboutVisualizer = new QAction( this );
    aboutVisualizer->setObjectName("main help about");
    aboutVisualizer->setText( tr( "About Visualizer" ) );
    helpMenu->addAction( aboutVisualizer );
    menubar->addMenu( helpMenu );
    menubar->actions().last()->setObjectName("main help");

    // signals
    connect( visualizerAssistant, SIGNAL( triggered() ), this, SLOT( showMainVisualizerAssistant() ) );
    connect( visualizerHelp, SIGNAL( triggered() ), this, SLOT( showVisualizerHelp() ) );
    connect( aboutVisualizer, SIGNAL( triggered() ), this, SLOT( showAboutVisualizer() ) );
}


// Function to send disconnect sut to listener (slot)
bool MainWindow::disconnectSUT()
{
    bool result = true;
    QString status = "SUT disconnected";

    if ( !execute_command( commandDisconnectSUT,
                           QString( activeDevice.value( "name" ) + " disconnect" ),
                           activeDevice.value( "name" )) ) {
        status = "SUT disconnecting failed";
        result = false;
    }

    statusbar( status, 2000 );
    emit disconnectSUTResult(result);
    return result;
}


bool MainWindow::disconnectExclusiveSUT()
{
    if ( activeDevice.value( "type" ).contains( "s60", Qt::CaseInsensitive ) ) {
        return disconnectSUT();
    }
    else {
        emit disconnectSUTResult(false);
        return false;
    }
}


void MainWindow::openFontDialog() {

    bool ok;

    QFont font = QFontDialog::getFont( &ok, *defaultFont, this );

    if ( ok ) {
        // font is set to the font the user selected
        defaultFont->fromString( font.toString() );
        emit defaultFontSet(*defaultFont);

        refreshDataDisplay();

    }

}

// LoadFileDataEvent, which is called from button event or menu
// This loads the xml file selected, updates tree-view and tries to load an image with the same name ( except
// of course suffix .png )
void MainWindow::loadFileData() {

    // qDebug( "start loadFileData -->" );

    // retrieve the file to a QString fileName
    QString fileName = QFileDialog::getOpenFileName( this, tr( "Open XML-file" ), "", tr( "XML DUMP Files ( *.xml )" ) );

    if ( fileName != NULL ) {

        // update xml-treeview
        updateObjectTree( fileName );

        // create filename and send it to imagewidget
        fileName = fileName.section( '.', 0, -2 );
        fileName.append( ".png" );

        imageWidget->refreshImage( fileName, false );
        imageWidget->update();

    }

}

// Prompts the user and stores current SUT state in the specified archive/folder
void MainWindow::saveStateAsArchive()
{

    QString folderName = selectFolder( "Save current state to folder", "Folder", QFileDialog::AcceptSave );

    if ( !folderName.isEmpty() ) {

        if ( !createStateArchive( folderName ) ) {

            QMessageBox::warning( this, "Save as folder", "Failed when saving state in folder:\n" + folderName );

        }

    }

}


void MainWindow::openRecordWindow()
{
    if ( recordMenu->isEnabled() ) {

        if ( currentApplication.value( "id" ).isEmpty() ) {

            QMessageBox::warning( this, tr( "TDriver Visualizer Recorder" ), "In order to start recording a target application must be selected\nfrom applications menu.\n\nPlease select one and try again." );

        } else {

            mRecorder->setOutputPath( outputPath );
            mRecorder->setActiveDevAndApp( activeDevice.value("name"), currentApplication.value( "id" ) );
            mRecorder->show();

            mRecorder->activateWindow();

        }

    }
}

// Handle application menu selection and deselection
void MainWindow::appSelected() {

    // qDebug() << "MainWindow::appSelected()";

    QAction *action = qobject_cast<QAction *>( sender() );

    if ( action ) {

        if( action->isChecked() == true ) {
            // if the action is not already checked, check it and uncheck all other actions
            QList<QAction *> menuActions;

            menuActions = appsMenu->actions();

            for ( int i = 0; i < menuActions.size(); i++ ) {
                menuActions.at( i )->setChecked( false );
            }

            QString processId = applicationsProcessIdMap.value( ( int )( action ) );
            currentApplication.clear();
            currentApplication.insert( "name", applicationsHash.value( processId ) );
            currentApplication.insert( "id",   processId );
            action->setChecked( true );
            foregroundApplication = ( processId == "0" ) ? true : false;
            // application changed, perform refresh
            refreshData();

        } else {
            // item already selected - keep as checked
            action->setChecked( true );
            updateWindowTitle();
        }
    }
}


// Helper function, called when device is selected from list
void MainWindow::deviceSelected() {

    QAction *action = qobject_cast<QAction *>( sender() );

    if ( action ) {

        QString strOldDevice = activeDevice.value("name");

        setActiveDevice( action->text() );

        if ( strOldDevice != activeDevice.value("name") ) {

            // clear application menu
            resetApplicationsList();

            // clear current application hash
            currentApplication.clear();

            // disable applications menu
            appsMenu->setDisabled( true );

            // empty current image
            imageWidget->clearImage();

            // clear object tree mappings
            clearObjectTreeMappings();

            // empty object tree
            objectTree->clear();

            // empty properties table
            clearPropertiesTableContents();

        }

    }

    QString deviceType = activeDevice.value("type");

    // enable s60 buttons selection if device type is 'kind of' s60
    viewButtons->setEnabled( deviceType.contains( "s60", Qt::CaseInsensitive ) );

    // enable api tab if if device type is 'kind of' qt
    tabWidget->setTabEnabled( tabWidget->indexOf( apiTab ), deviceType.contains( "qt", Qt::CaseInsensitive ) );
    apiFixtureEnabled = deviceType.contains( "qt", Qt::CaseInsensitive );
    apiFixtureChecked = false;

    // enable recording menu if if device type is 'kind of' qt
    recordMenu->setEnabled( deviceType.contains( "qt", Qt::CaseInsensitive ) && !applicationsHash.empty() );

    // update window title
    updateWindowTitle();

    behavioursMap.clear();

    propertyTabLastTimeUpdated.clear();

}

// Creates a folder containing xml and png dump using the specified file path.
bool MainWindow::createStateArchive( QString targetPath )
{

    QStringList sourceFiles;

    bool filesExist( false );

    QString filename = "/visualizer_dump_" + activeDevice.value("name");

    sourceFiles << filename + ".xml" << filename + ".png";

    foreach( QString sourceFile, sourceFiles ) { if ( QFile::exists( targetPath + "/" + sourceFile ) ) { filesExist = true; } }

    if ( filesExist ) {

        QMessageBox::StandardButton selectedButton =
                QMessageBox::question(
                        this,
                        "Overwrite?",
                        "Files already exist in the target folder and will be overwritten, do you wish to continue?",
                        QMessageBox::Yes | QMessageBox::No,
                        QMessageBox::No
                        );


        if ( selectedButton == QMessageBox::No ) { return true; }
    }

    foreach( QString sourceFile, sourceFiles ) {

        if ( QFile::exists( targetPath + "/" + sourceFile ) ) { QFile::remove( targetPath + "/" + sourceFile );    }
        if ( !QFile::copy( outputPath + sourceFile, targetPath + "/" + sourceFile ) ) { return false; }

    }

    return true;

}

// This function is used to get the file name for tdriver_parameters.xml file (activated from menu)
void MainWindow::getParameterXML() {

    QString fileName = QFileDialog::getOpenFileName( this, "Open tdriver_parameters.xml", "", "File: (tdriver_parameters.xml)" );

    if ( fileName != NULL ) {
        getDevicesList( fileName );
        //parseBehavioursXml( fileName );
    }

}

void MainWindow::updateDevicesList() {

    int count = 0;

    deviceMenu->clear();

    QAction *deviceActionList[ deviceList.size() ];

    QMapIterator<QString, QHash<QString, QString> > iterator( deviceList );

    while ( iterator.hasNext() ) {

        iterator.next();

        deviceActionList[ count ] = new QAction( deviceMenu );
        deviceActionList[ count ]->setObjectName("main device "+iterator.key());
        deviceActionList[ count ]->setText( iterator.key() );
        deviceActionList[ count ]->setToolTip( iterator.value().value( "type" ) );

        connect( deviceActionList[ count ], SIGNAL( triggered() ), this, SLOT( deviceSelected() ) );

        deviceMenu->addAction( deviceActionList[ count ] );

        count += 1;
    }

    // disable devices menu, disconnect sut etc if no devices found
    deviceMenu->setEnabled( count != 0 );
    disconnectCurrentSUT->setEnabled( count != 0 );

}



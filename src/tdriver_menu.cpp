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
#include "tdriver_image_view.h"
#include "tdriver_recorder.h"
#include "tdriver_debug_macros.h"

#include <QSharedPointer>
#include <QShortcut>
#include <QMenu>
#include <QAction>

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
    fileMenu->addAction(parseSUT);

    fileMenu->addSeparator();

    // Load state xml file
    fileMenu->addAction( loadXmlAction );

    // save state xml
    fileMenu->addAction( saveStateAction );

    // font

    fileMenu->addSeparator();
    fileMenu->addAction( fontAction );

    fileMenu->addSeparator();
    // refresh

    fileMenu->addAction( refreshAction );
    fileMenu->addAction( delayedRefreshAction );

    // tap and auto-refresh on Image View click
    //note:  action constructed in MainWindow::createImageViewDockWidget()

    // disconnect

    fileMenu->addAction( disconnectCurrentSUT );
    fileMenu->addSeparator();

    // exit

    fileMenu->addAction(exitAction);

    menubar->addMenu( fileMenu );
    menubar->actions().last()->setObjectName("main file");
}

void MainWindow::createSearchMenu() {

    searchMenu = new QMenu( " &Search ", this );
    searchMenu->setObjectName("main search");

    // dockable widget menu: clipboard

    searchMenu->addAction( findAction );

    menubar->addMenu( searchMenu );
    menubar->actions().last()->setObjectName("main search");

}

void MainWindow::createViewMenu() {

    // menu

    viewMenu = new QMenu( " &View ", this );
    viewMenu->setObjectName("main view");

    // dock and toolbar submenu will be inserted as the first element of this menu

    //viewMenu->addSeparator();

    // object tree menu: expand all

    viewMenu->addAction( viewExpandAll );

    // object tree menu: collapse all

    viewMenu->addAction( viewCollapseAll );

    viewMenu->addSeparator();

    // Show XML

    viewMenu->addAction( showXmlAction );

    menubar->addMenu( viewMenu );
    menubar->actions().last()->setObjectName("main view");

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
    //appsMenu->setDisabled( true );
    appsMenu->addAction(startAppAction);
    appsMenu->addSeparator();
    menubar->addMenu( appsMenu );
    menubar->actions().last()->setObjectName("main applications");
}

void MainWindow::createRecordMenu() {

    // menu

    recordMenu = new QMenu( tr( " &Record " ), this );
    recordMenu->setObjectName("main record");
    recordMenu->setDisabled( true );

    // open record dialog

    recordMenu->addAction( recordAction );
    menubar->addMenu( recordMenu );
    menubar->actions().last()->setObjectName("main record");
}



void MainWindow::createHelpMenu()
{
    // menu
    helpMenu = new QMenu( tr( " &Help " ), this );
    helpMenu->setObjectName("main help");

    // TDriver help
    helpMenu->addAction( visualizerAssistant );
    helpMenu->addAction( visualizerHelp );

    // Visualizer Help
    helpMenu->addSeparator();

    // About TDriver
    helpMenu->addAction( aboutVisualizer );

    menubar->addMenu( helpMenu );
    menubar->actions().last()->setObjectName("main help");
}


// Function to send disconnect sut to listener (slot)
bool MainWindow::disconnectSUT()
{
    bool result = true;
    QString status = "SUT disconnected";

    if ( !executeTDriverCommand( commandDisconnectSUT,
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
void MainWindow::loadFileData()
{
    static const QString imageSuffix(".png");

    QString dirName = applicationSettings->value(keyLastUiStateDir, QVariant("")).toString();
    // retrieve the file to a QString fileName
    QString fileName = QFileDialog::getOpenFileName( this, tr( "Open XML-file" ), dirName, tr( "XML DUMP Files ( *.xml )" ) );

    QFileInfo info(fileName);

    if (info.isFile()) {
        applicationSettings->setValue(keyLastUiStateDir, info.absolutePath());

        // update xml-treeview
        updateObjectTree( fileName );

        // create image filename and send it to imagewidget
        fileName.replace(fileName.lastIndexOf('.'), fileName.size(), imageSuffix);
        imageWidget->refreshImage( fileName );
    }
}


// Prompts the user and stores current SUT state in the specified archive/folder
void MainWindow::saveStateAsArchive()
{

    QString folderName = selectFolder( "Save current state to folder", "Folder", QFileDialog::AcceptSave, keyLastUiStateDir );

    if ( !folderName.isEmpty() ) {

        if ( !createStateArchive( folderName ) ) {

            QMessageBox::warning( this, "Save as folder", "Failed when saving state in folder:\n" + folderName );

        }

    }

}


void MainWindow::openRecordWindow()
{
    if ( recordMenu->isEnabled() ) {

        if (currentApplication.isNull()) {

            QMessageBox::warning( this, tr( "TDriver Visualizer Recorder" ), "In order to start recording a target application must be selected\nfrom applications menu.\n\nPlease select one and try again." );

        } else {

            mRecorder->setActiveDevAndApp( activeDevice.value("name"), currentApplication.id );
            mRecorder->show();

            mRecorder->activateWindow();

        }

    }
}

// Handle application menu selection and deselection
void MainWindow::appSelected() {

    // qDebug() << "MainWindow::appSelected()";

    QAction *action = qobject_cast<QAction *>( sender() );

    if ( action && applicationsActionMap.contains(action)) {

        QList<QAction *> menuActions(appsMenu->actions());
        for ( int i = 0; i < menuActions.size(); i++ ) {
            menuActions.at( i )->setChecked( menuActions.at(i) == action );
        }

        QString processId = applicationsActionMap.value( action );
        currentApplication.set(processId, applicationsNamesMap.value( processId ));

        action->setChecked( true );
        foregroundApplication = ( processId == "0" ) ? true : false;
        updateWindowTitle();
        refreshData();
    }
}


// Helper function, called when device is selected from list
void MainWindow::deviceSelected() {

    QAction *action = qobject_cast<QAction *>( sender() );

    if ( action ) {

        QString strOldDevice = activeDevice.value("name");

        setActiveDevice( action->text() );

        if ( strOldDevice != activeDevice.value("name") ) {

            // clear applications
            resetApplicationsList();
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

#if DEVICE_BUTTONS_ENABLED
    // enable s60 buttons selection if device type is 'kind of' s60
    viewButtons->setEnabled( deviceType.contains( "s60", Qt::CaseInsensitive ) );
#endif

    // enable api tab if if device type is 'kind of' qt
    tabWidget->setTabEnabled( tabWidget->indexOf( apiTab ), deviceType.contains( "qt", Qt::CaseInsensitive ) );
    apiFixtureEnabled = deviceType.contains( "qt", Qt::CaseInsensitive );
    apiFixtureChecked = false;

    // enable recording menu if if device type is 'kind of' qt
    recordMenu->setEnabled( deviceType.contains( "qt", Qt::CaseInsensitive )
                           && !applicationsNamesMap.empty() );

    // update window title
    updateWindowTitle();

    behavioursMap.clear();

    propertyTabLastTimeUpdated.clear();

}

// Creates a folder containing xml and png dump using the specified file path.
bool MainWindow::createStateArchive( QString targetPath )
{
    QStringList sourceFiles;
    sourceFiles << imageWidget->lastImageFileName() << uiDumpFileName;

    QStringList targetFiles;

    QStringList problemFiles;

    int ii;
    int count = sourceFiles.size();

    if (!targetPath.endsWith('/'))
        targetPath.append('/');

    for (ii=0; ii < count; ++ii) {
        QString targetFile = targetPath + QFileInfo(sourceFiles.at(ii)).fileName();
        targetFile.replace(QRegExp("_\\d+(\\.[a-zA-Z0-9_]+)$"), "\\1");
        if ( QFileInfo(targetFile) != QFileInfo(sourceFiles.at(ii)) && QFile::exists(targetFile)) {
            problemFiles << targetFile;
        }
        targetFiles << targetFile;
    }

    if ( !problemFiles.isEmpty() ) {

        QMessageBox::StandardButton selectedButton =
                QMessageBox::question(
                    this,
                    "Overwrite?",
                    "Following target files already exist, do you wish to overwrite?\n\n  " + problemFiles.join("\n  "),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);
        if ( selectedButton == QMessageBox::No ) {
            return true;
        }
    }

    for (ii=0; ii < count; ++ii) {
        if (QFileInfo(targetFiles.at(ii)) != QFileInfo(sourceFiles.at(ii))) {
            bool result;
            if ( QFile::exists( targetFiles.at(ii))) {
                result = QFile::remove( targetFiles.at(ii) );
                qDebug() << FCFL << "QFile::remove('" << targetFiles.at(ii) <<"') ==" << result;
            }

            result = QFile::copy( sourceFiles.at(ii), targetFiles.at(ii));
            qDebug() << FCFL << "QFile::copy('" << sourceFiles.at(ii) << "', '" << targetFiles.at(ii) << "'') ==" << result;
            if ( !result) {
                problemFiles << (sourceFiles.at(ii) + " => " + targetFiles.at(ii));
            }
        }
        else qDebug() << FCFL << "Skipping copying file to itself:" << sourceFiles.at(ii);
    }

    if ( !problemFiles.isEmpty() ) {

        QMessageBox::warning(
                    this,
                    "Failed to copy some files!",
                    "Following files failed to copy:\n\n  " + problemFiles.join("\n  "));

        return false;
    }

    return true;

}

// This function is used to get the file name for tdriver_parameters.xml file (activated from menu)
void MainWindow::getParameterXML()
{
    QString dirName = applicationSettings->value(keyLastTDriverDir, QVariant("")).toString();
    QString fileName = QFileDialog::getOpenFileName( this, "Open tdriver_parameters.xml", dirName, "File: (tdriver_parameters.xml)" );

    QFileInfo info(fileName);
    if (info.isFile()) {
        applicationSettings->setValue(keyLastTDriverDir, info.absolutePath());

        if (getXmlParameters( fileName )) {
            // parametersFile changed
            parametersFile = fileName;
        }
    }
}


void MainWindow::updateDevicesList(const QMap<QString, QHash<QString, QString> > &newDeviceList)
{
    // clear previous list of actions
    while(!deviceMenu->isEmpty()) {
        QAction *delAct = deviceMenu->actions().first();
        deviceMenu->removeAction(delAct);
        delAct->deleteLater();
    }

    deviceList = newDeviceList;
    for ( QMap<QString, QHash<QString, QString> >::const_iterator iterator(newDeviceList.constBegin());
         iterator != newDeviceList.constEnd();
         ++iterator) {

        QAction *devAction = new QAction(this);
        devAction->setObjectName("main device "+iterator.key());
        devAction->setText( iterator.key() );
        devAction->setToolTip( iterator.value().value( "type" ) );

        connect( devAction, SIGNAL( triggered() ), this, SLOT( deviceSelected() ) );

        deviceMenu->addAction( devAction );
    }

    // disable devices menu, disconnect sut etc if no devices found
    deviceMenu->setDisabled(deviceMenu->isEmpty());
    disconnectCurrentSUT->setEnabled(deviceMenu->isEmpty());
}



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
#include <QDir>

static const QString imageSuffix(".png");

void MainWindow::createTopMenuBar() {

    menubar = new QMenuBar();
    menubar->setObjectName( "main menubar" );

    createFileMenu();

    editMenu = new QMenu( tr( "&Edit" ), this );
    fileMenu->setObjectName("main edit");
    menubar->addMenu(editMenu)->setObjectName("main edit");

    createViewMenu();
    createSearchMenu();
    createDevicesMenu();
    createApplicationsMenu();
    createRecordMenu();
    createHelpMenu();
}


void MainWindow::createFileMenu() {

    // menu

    fileMenu = new QMenu( tr( "&File" ), this );
    fileMenu->setObjectName("main file");


    // parse tdriver parameters xml
    fileMenu->addAction(parseSUT);

    fileMenu->addSeparator();

    // Load state xml file
    fileMenu->addAction( loadXmlAction );

    // save state xml
    fileMenu->addAction( saveStateAction );

    // state history submenu
    fileMenu->addAction( stateHistoryAction );

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

    fileMenu->addAction( sutDisconnectAction );
    fileMenu->addSeparator();

    // exit

    fileMenu->addAction(exitAction);

    menubar->addMenu( fileMenu )->setObjectName("main file");
}

void MainWindow::createSearchMenu() {

    searchMenu = new QMenu( "&Search", this );
    searchMenu->setObjectName("main search");

    // dockable widget menu: clipboard

    searchMenu->addAction( findAction );

    menubar->addMenu( searchMenu )->setObjectName("main search");

}

void MainWindow::createViewMenu() {

    // menu

    viewMenu = new QMenu( "&View", this );
    viewMenu->setObjectName("main view");

    // dock and toolbar submenu will be inserted as the first element of this menu

    //viewMenu->addSeparator();

    // object tree menu: expand all

    viewMenu->addAction( viewExpandAll );

    // object tree menu: collapse all

    viewMenu->addAction( viewCollapseAll );

    {
        QMenu *menu = viewMenu->addMenu(tr("Layouts"));
        menu->addActions(saveLayoutActions);
        menu->addSeparator();
        menu->addActions(restoreLayoutActions);
        menu->addSeparator();
        menu->addAction(restoreDefaultLayoutAction);
    }

    viewMenu->addSeparator();

    // Show XML

    viewMenu->addAction( showXmlAction );

    menubar->addMenu( viewMenu )->setObjectName("main view");

}

void MainWindow::createDevicesMenu() {

    // menu

    deviceMenu = new QMenu( tr( "&Devices" ), this );
    deviceMenu->setObjectName("main device");
    deviceMenu->setDisabled( true );
    menubar->addMenu( deviceMenu )->setObjectName("main device");

}

void MainWindow::createApplicationsMenu() {

    // menu

    appsMenu = new QMenu( tr( "&Applications" ), this );
    appsMenu->setObjectName("main applications");
    //appsMenu->setDisabled( true ); // Now we have extra item in the menu so always show
    appsMenu->addAction(startAppAction);
    appsMenu->addAction(appsRefreshAction);
    appsMenu->addSeparator();
    menubar->addMenu( appsMenu )->setObjectName("main applications");
}

void MainWindow::createRecordMenu() {

    // menu

    recordMenu = new QMenu( tr( "&Record" ), this );
    recordMenu->setObjectName("main record");
    recordMenu->setDisabled( true );

    // open record dialog

    recordMenu->addAction( recordAction );
    menubar->addMenu( recordMenu )->setObjectName("main record");
}



void MainWindow::createHelpMenu()
{
    // menu
    helpMenu = new QMenu( tr( "&Help" ), this );
    helpMenu->setObjectName("main help");

    // TDriver help
    helpMenu->addAction( visualizerAssistant );
    helpMenu->addAction( visualizerHelp );

    // Visualizer Help
    helpMenu->addSeparator();

    // About TDriver
    helpMenu->addAction( aboutVisualizer );

    menubar->addMenu( helpMenu )->setObjectName("main help");
}


// Function to send disconnect sut to listener (slot)
bool MainWindow::disconnectSUT()
{
    bool result = false;

    if (activeDevice.isEmpty()) {
        qDebug() << FCFL << "trying to disconnect empty activeSut";
        emit disconnectionOk(true);
    }
    else {
        result =  sendTDriverCommand(commandDisconnectSUT,
                                     QStringList() << activeDevice << "disconnect",
                                     tr("disconnect '%1'").arg(activeDevice));
        if (result) {
            statusbar( tr("Sent SUT disconnect...") );
        }
        else {
            statusbar( tr("Failed to send SUT disconnect command!"), 2000 );
            emit disconnectionOk(true);
        }
    }

    return result;
}


bool MainWindow::disconnectExclusiveSUT()
{
    if ( TDriverUtil::isExclusiveConnectionSut(activeDeviceParams.value( "type" ))) {
        qDebug() << FCFL << "Disconnecting SUT type" << activeDeviceParams.value( "type" );
        return disconnectSUT();
    }
    else {
        qDebug() << FCFL << "Skipping non-exclusive SUT type" << activeDeviceParams.value( "type" );
        emit disconnectionOk(true);
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

        refreshAppearance();

    }

}


// LoadFileDataEvent, which is called from button event or menu
// This opens a dialog, loads the xml file selected, updates tree-view,
// then tries to laod corresponding image file.
void MainWindow::loadStateByDialog()
{
    QSettings settings;

    QString dirName = settings.value(keyLastUiStateDir, QVariant("")).toString();
    // retrieve the file to a QString fileName
    QString fileName = QFileDialog::getOpenFileName( this, tr( "Open XML-file" ), dirName, tr( "XML DUMP Files ( *.xml )" ) );

    QFileInfo info(fileName);

    if (info.isFile()) {
        settings.setValue(keyLastUiStateDir, info.absolutePath());

        // update xml-treeview
        titleFileText = fileName;
        updateObjectTree( fileName );

        // create image filename and send it to imagewidget
        fileName.replace(fileName.lastIndexOf('.'), fileName.size(), imageSuffix);
        imageWidget->refreshImage( fileName );

        updateWindowTitle();
    }
}


void MainWindow::loadStateFromHistoryDir(const QString &dirPath)
{
    loadStateFromDir(dirPath);
    titleFileText = tr("previous state %1").arg(dirPath.mid(dirPath.lastIndexOf('_'), 2));
    updateWindowTitle();

}


void MainWindow::loadStateFromDir(const QString &dirPath)
{
    QDir dir(dirPath);

    QString errText;

    if (!dir.exists()) {
        errText = tr("There is no directory with path %1").arg(dirPath);
    }
    else {
        QStringList xmlFiles = dir.entryList(QStringList() << "*.xml", QDir::Files);
        qDebug() << FCFL << dirPath << "contains:" << xmlFiles;
        if (xmlFiles.isEmpty()) {
            errText = tr("Directory %1 contains no .xml files").arg(dirPath);
        }
        else if (xmlFiles.size() > 1) {
            errText = tr("Directory %1 contains multiple .xml files:\n\n  %2")
                    .arg(dirPath).arg(xmlFiles.join("\n  "));
        }
        else {
            QString filePath = dirPath + "/" + xmlFiles.first();
            updateObjectTree(filePath);

            filePath.replace(filePath.lastIndexOf('.'), filePath.size(), imageSuffix);
            imageWidget->refreshImage(filePath);

        }
    }
    if (!errText.isEmpty()) {
        QMessageBox::warning(this,
                             tr("Loading State from a directory"),
                             errText);
    }
}


static inline bool recursiveRemove(QString path) {
    QFileInfo info(path);
    if (!info.exists()) return false; // shortcut out

    QString fn = info.fileName();
    if (fn.isEmpty() || fn == ".." || fn == ".") return false;

    if (info.isDir() && !info.isSymLink()) {
        QDir dir(path);
        if (!path.endsWith('/')) path.append('/');
        foreach(const QString &entryName,
                dir.entryList(/*QDir::NoDotAndDotDot*/)) {
            recursiveRemove(path + entryName);
        }
        path.chop(1); // remove trailing /, added above, just in case
        if (!QDir().rmdir(path)) return false;
    }
    else {
        QFile file(path);
        if (!file.remove()) {
            QString err = file.errorString();
            return false;
        }
    }

    return true;
}


static inline QString filledDigitString(unsigned num, int fieldWidth) {
    static const QString argTemplate("%1");
    static const QChar fillChar('0');
    return argTemplate.arg(num, fieldWidth, 10, fillChar);
}


void MainWindow::historySaveCurrentState()
{
    QSettings settings;

    unsigned dirCount = settings.value(keyHistoryStateDirCount, QVariant(8)).toUInt();

    if (dirCount == 0) return; // disabled
    if (dirCount > 99) dirCount = 99; // sanity check

    unsigned ind;

    // remove excess history folders
    ind = dirCount;
    forever {
        if (ind > 99) break; // sanity check
        QString filePath = stateHistoryFilePathPrefix + filledDigitString(ind, 2);
        if (!recursiveRemove(filePath)) break; // stop when remove fails
        qDebug() << FCFL << "Removed directory" << filePath;
        ++ind;
    }

    //QString namePrefix = stateHistoryFilePathPrefix.mid(stateHistoryFilePathPrefix.lastIndexOf('/') + 1);

    // shift history folder names
    for(ind = dirCount; ind > 0; --ind) {
        QString oldName = stateHistoryFilePathPrefix + filledDigitString(ind-1, 2);
        QString newName = stateHistoryFilePathPrefix + filledDigitString(ind, 2);
        QFile file(oldName);
        if (file.exists() && !file.rename(newName)) {
            QString err = file.errorString();
            QString fn = file.fileName();
            qDebug() << FCFL << "Failed to rotate" << oldName << "->" << newName << ":" << err << "(ignored)";
        }
        // else file didn't exist, or rename succeeded
    }

    QString name =  stateHistoryFilePathPrefix + "00";
    if (!QDir().mkdir(name)) {
        qDebug() << FCFL << "Failed to create new directory" << name;
    }
    else if (!createStateArchive(name)) {
        qDebug() << FCFL << "Failed to store state history to dir" << name;
    }
}


// Prompts the user and stores current SUT state in the specified archive/folder
void MainWindow::saveStateAsArchive()
{

    QString folderName = selectFolder( "Save current state to folder", "Folder", QFileDialog::AcceptSave, keyLastUiStateDir );

    if ( !folderName.isEmpty() ) {

        if ( !createStateArchive( folderName ) ) {

            QMessageBox::warning(this,
                                 tr("Save as folder"),
                                 tr("Failed when saving state in folder:\n") + folderName );

        }

    }

}


void MainWindow::openRecordWindow()
{
    if ( recordMenu->isEnabled() ) {

        if (!currentApplication.useId()) {

            QMessageBox::warning(this,
                                 tr("TDriver Visualizer Recorder" ),
                                 tr("In order to start recording a target application must be selected\n"
                                    "from applications menu.\n\nPlease select one and try again."));

        } else {

            mRecorder->setActiveDevAndApp( activeDevice, currentApplication.id );
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
        currentApplication.setForeground((processId == "0")
                                         /*|| TDriverUtil::isSymbianSut(activeDeviceParams.value( "type" ))*/);
        sendAppListRequest(true);
    }
    updateWindowTitle();
}


// Helper function, called when device is selected from list
void MainWindow::deviceSelected()
{
    QAction *action = qobject_cast<QAction *>( sender() );

    if ( action ) {

        QString strOldDevice = activeDevice;

        setActiveDevice( action->text() );

        if ( strOldDevice != activeDevice) {

            // clear applications
            resetMessageSequenceFlags();
            resetApplicationsList();
            currentApplication.clearInfo();

            // disable applications menu
            //appsMenu->setDisabled( true ); // Now we have extra item in the menu so always show

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

    if (!activeDevice.isEmpty()) {

        bool deviceIsQt = TDriverUtil::isQtSut(activeDeviceParams.value("type"));

#if DEVICE_BUTTONS_ENABLED
        // enable s60 buttons selection if device type is 'kind of' symbian
        viewButtons->setEnabled( deviceType.contains( "symbian", Qt::CaseInsensitive ) );
#endif

#if DISABLE_API_TAB_PENDING_REMOVAL
        apiFixtureEnabled = false;
        apiFixtureChecked = true;
#else
        // enable api tab if if device type is 'kind of' qt
        tabWidget->setTabEnabled( tabWidget->indexOf( apiTab ), deviceIsQt);
        apiFixtureEnabled = deviceIsQt;
        apiFixtureChecked = false;
#endif

        // enable recording menu if device type is 'kind of' qt
        recordMenu->setEnabled( deviceIsQt && !applicationsNamesMap.empty() );
        sendAppListRequest(false);
    }
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

    QStringList problemList;

    int ii;
    int count = sourceFiles.size();

    if (!targetPath.endsWith('/'))
        targetPath.append('/');

    for (ii=0; ii < count; ++ii) {
        QString targetFile = targetPath + QFileInfo(sourceFiles.at(ii)).fileName();
        targetFile.replace(QRegExp("_\\d+(\\.[a-zA-Z0-9_]+)$"), "\\1");
        if ( QFileInfo(targetFile) != QFileInfo(sourceFiles.at(ii)) && QFile::exists(targetFile)) {
            problemList << targetFile;
        }
        targetFiles << targetFile;
    }    

    if ( !problemList.isEmpty() ) {

        QMessageBox::StandardButton selectedButton =
                QMessageBox::question(
                    this,
                    tr("Overwrite?"),
                    tr("Following target files already exist, do you wish to overwrite?\n\n  ") + problemList.join("\n  "),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);
        if ( selectedButton == QMessageBox::No ) {
            return true;
        }

        problemList.clear();
    }

    for (ii=0; ii < count; ++ii) {
        if (QFileInfo(targetFiles.at(ii)) != QFileInfo(sourceFiles.at(ii))) {
            bool result;
            if ( QFile::exists( targetFiles.at(ii))) {
                result = QFile::remove( targetFiles.at(ii) );
                qDebug() << FCFL << "QFile::remove('" << targetFiles.at(ii) <<"') ==" << result;
            }

            QFile source(sourceFiles.at(ii));
            result = source.copy(targetFiles.at(ii));
            qDebug() << FCFL << "QFile::copy('" << sourceFiles.at(ii) << "', '" << targetFiles.at(ii) << "'') ==" << result;
            if ( !result) {
                problemList << tr("\n%1 => %2 (%3)")
                               .arg(sourceFiles.at(ii), targetFiles.at(ii), source.errorString());
            }
        }
        else qDebug() << FCFL << "Skipping copying file to itself:" << sourceFiles.at(ii);
    }

    if ( !problemList.isEmpty() ) {

        QMessageBox::warning(
                    this,
                    tr("Failed to copy some files!"),
                    tr("Following files failed to copy:\n" )+ problemList.join("\n"));

        return false;
    }

    return true;
}


// This function is used to get the file name for tdriver_parameters.xml file (activated from menu)
void MainWindow::getParameterXML()
{
    QSettings settings;
    QString dirName = settings.value(keyLastTDriverDir, QVariant("")).toString();
    QString fileName = QFileDialog::getOpenFileName( this, "Open tdriver_parameters.xml", dirName, "File: (tdriver_parameters.xml)" );

    QFileInfo info(fileName);
    if (info.isFile()) {
        settings.setValue(keyLastTDriverDir, info.absolutePath());

        if (getXmlParameters( fileName )) {
            // parametersFile changed
            parametersFile = fileName;
        }
    }
}


void MainWindow::updateDevicesList(const QStringList &newDeviceList)
{
    // clear previous list of actions
    while(!deviceMenu->isEmpty()) {
        QAction *delAct = deviceMenu->actions().first();
        deviceMenu->removeAction(delAct);
        delAct->deleteLater();
    }

    deviceList = newDeviceList;
    if (deviceList.isEmpty()) {
        QAction *emptyAct = new QAction(tr("No devices!"), this);
        emptyAct->setDisabled(true);
        deviceMenu->addAction( emptyAct );
    }
    else {
        foreach(const QString &name, deviceList) {
            QAction *devAction = new QAction(this);
            devAction->setObjectName("main device " + name);
            devAction->setText( name );
            connect( devAction, SIGNAL( triggered() ), this, SLOT( deviceSelected() ) );
            deviceMenu->addAction( devAction );
        }
    }

    sutDisconnectAction->setEnabled(deviceList.isEmpty());
}

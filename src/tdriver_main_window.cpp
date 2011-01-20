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
#include "tdriver_recorder.h"

#include <tdriver_tabbededitor.h>
#include <tdriver_rubyinterface.h>
#include "../common/version.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QErrorMessage>
#include <QThread>
#include <QToolBar>
#include <QLabel>

#include <tdriver_debug_macros.h>


MainWindow::MainWindow() :
    QMainWindow(),
    foregroundApplication(true),
    tdriverMsgBox(new QErrorMessage(this)),
    tdriverMsgTotal(0),
    tdriverMsgShown(1),
    keyLastUiStateDir("files/last_uistate_dir"),
    keyLastTDriverDir("files/last_tdriver_dir")
{
    // ugly hack to disable the "don't show again" checkbox,
    // may not work in future Qt versions, and may not work on all platforms
    foreach(QObject *child, tdriverMsgBox->children()) {
        QCheckBox *cb = qobject_cast<QCheckBox*>(child);
        if (cb) {
            //qDebug() << FCFL << "hiding tdriverMsgBox QCheckBox" << cb->text();
            cb->hide();
            cb->setDisabled(true);
            cb->setCheckState(Qt::Checked);
        }
        else {
            QPushButton *pb = qobject_cast<QPushButton*>(child);
            if (pb) {
                //qDebug() << FCFL << "connecting tdriverMsgBox QPushButton" << pb->text();
                connect(pb, SIGNAL(clicked()), this, SLOT(tdriverMsgOkClicked()));
            }
        }
    }

    tdriverMsgBox->resize(600, 400);
    tdriverMsgBox->setSizeGripEnabled(true);
    tdriverMsgSetTitleText();
    connect(tdriverMsgBox, SIGNAL(finished(int)), this, SLOT(tdriverMsgFinished()));
}


void MainWindow::tdriverMsgSetTitleText()
{
    tdriverMsgBox->setWindowTitle(tr("TDriver Notification %1/%2").
                                  arg(qMin(tdriverMsgShown, tdriverMsgTotal)).
                                  arg(tdriverMsgTotal));
    //qDebug() << FCFL << "set tdriverMsgBox title to" << tdriverMsgBox->windowTitle();
}


void MainWindow::tdriverMsgOkClicked()
{
    ++tdriverMsgShown;
    tdriverMsgSetTitleText();
    tdriverMsgBox->repaint();
}


void MainWindow::tdriverMsgFinished()
{
    // verify that tdriverMsgShown is not out of sync
    if (tdriverMsgShown < tdriverMsgTotal) {
        tdriverMsgShown = tdriverMsgTotal;
        tdriverMsgSetTitleText();
    }
}


void MainWindow::tdriverMsgAppend(QString message)
{
    message.replace('\n', QChar::ParagraphSeparator);
    ++tdriverMsgTotal;
    tdriverMsgSetTitleText();
    tdriverMsgBox->showMessage(message);
    tdriverMsgBox->raise();
    tdriverMsgBox->repaint();
}

// Performs post-initialization checking.
void MainWindow::checkInit()
{
    // check if RUBYOPT has been set
    QString current( getenv( tr( "RUBYOPT" ).toLatin1().data() ) );
    if ( current != "rubygems" ) { putenv( tr( "RUBYOPT=rubygems" ).toLatin1().data() ); }
}


bool MainWindow::checkVersion( QString currentVersion, QString requiredVersion )
{
    // convert required version to array
    QStringList tmpRequiredVersionArray = requiredVersion.split( QRegExp("[.-]") );

    // convert installed driver version to array
    QStringList tmpDriverVersionArray = currentVersion.split( QRegExp("[.-]") );

    // make version arrays same length
    while ( tmpDriverVersionArray.count() < tmpRequiredVersionArray.count() ) {
        tmpDriverVersionArray.append("0");
    }

    bool versionOk = true;

    for ( int index = 0; index < tmpRequiredVersionArray.count();  index++ ) {
        int current_version = tmpDriverVersionArray.at( index ).toInt();
        int required_version = tmpRequiredVersionArray.at( index ).toInt();

        // check if installed version is new enough
        if ( current_version > required_version ) {
            break;
        }
        else {
            if ( current_version < required_version ){
                versionOk = false;
                break;
            }
        }
    }

    return versionOk;
}


// Function entered when creating Visulizer. Sets default and calls helper functions
// to setup UI of Visualizer
bool MainWindow::setup()
{
    setObjectName("main");
    QTime t;  // for performance debugging, can be removed

    // read visualizer settings from visualizer.ini file
    applicationSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "Nokia", "TDriver_Visualizer");

    TDriverRubyInterface::startGlobalInstance();
    connect(TDriverRubyInterface::globalInstance(), SIGNAL(rbiError(QString,QString,QString)),
            this, SLOT(handleRbiError(QString,QString,QString)));

    // determine if connection to TDriver established -- if not, allow user to run TDriver Visualizer in viewer/offline mode
    offlineMode = true;
    t.start();
    QString goOnlineError;
    if (!(goOnlineError = TDriverRubyInterface::globalInstance()->goOnline()).isNull()) {
        tdriverMsgAppend(tr("TDriver Visualizer failed to interface with TDriver framework:\n\n" )
                    + goOnlineError
                    + tr("\n\n=== Launching in offline mode ==="));
    }
    else {
        QString installedDriverVersion = getDriverVersionNumber();

        if ( !checkVersion( installedDriverVersion, REQUIRED_DRIVER_VERSION ) ) {
            tdriverMsgAppend(tr("TDriver Visualizer is not compatible with this version of TDriver. Please update your TDriver environment.\n\n") +
                             tr("Installed version: ") + installedDriverVersion +
                             tr("\nRequired version: ") + REQUIRED_DRIVER_VERSION + tr(" or later")+
                             tr("\n\n=== Launching in offline mode ===")
                             );
        }
        else {
            offlineMode = false; // TDriver successfully initialized!
        }
    }
    qDebug() << FCFL << "RBI goOnline  result" << !offlineMode << "secs" << float(t.elapsed())/1000.0;

    if (offlineMode) {
        qWarning("Failed to initialize TDriver, closing Ruby process");
        TDriverRubyInterface::globalInstance()->requestClose();
    }

    // default font for QTableWidgetItems and QTreeWidgetItems
    defaultFont = new QFont;
    defaultFont->fromString(  applicationSettings->value( "font/settings", QString("Sans Serif,8,-1,5,50,0,0,0,0,0") ).toString() );
    emit defaultFontSet(*defaultFont);

    // default window size & position
    QPoint windowPosition = applicationSettings->value( "window/pos", QPoint( 200, 200 ) ).toPoint();
    QSize windowSize = applicationSettings->value( "window/size", QSize( 950, 600 ) ).toSize();

    // read dock visibility settings
    bool imageVisible = applicationSettings->value( "view/image", true ).toBool();
    bool clipboardVisible = applicationSettings->value( "view/clipboard", true ).toBool();
    bool propertiesVisible = true; //applicationSettings->value( "view/properties", true ).toBool();
#if DEVICE_BUTTONS_ENABLED
    bool buttonVisible = applicationSettings->value( "view/buttons", false ).toBool();
#endif
    bool shortcutsVisible = applicationSettings->value( "view/shortcuts", false ).toBool();
    bool appsVisible = applicationSettings->value( "view/applications", false ).toBool();
    bool editorVisible = applicationSettings->value( "view/editor", false ).toBool();

    // default sut
    QString defaultDevice = applicationSettings->value( "sut/activesut", QString( "sut_qt" ) ).toString();

    // if default sut is empty (in ini file), set it as 'sut_qt'
    if ( defaultDevice.isEmpty() ) { defaultDevice = "sut_qt"; }

    // initialize & default settings

    tdriverPath = applicationSettings->value( "files/location", "" ).toString();
    if ( tdriverPath.isEmpty() ) {
        // construct QString tdriverPath depending on OS
#if (defined(Q_OS_WIN32))
        tdriverPath = "c:/tdriver/";
#else
        tdriverPath = "/etc/tdriver/";
#endif
    }

    // set current parameters xml file to be used
    while ( !QFile((parametersFile = tdriverPath + "/tdriver_parameters.xml")).exists() ) {

        QMessageBox::StandardButton result = QMessageBox::critical(
                0,
                tr("Missing file"),
                tr("Could not locate TDriver parameters file:\n\n  %1\n\n").arg(parametersFile) +
                tr("Please click Ok to select correct folder, or Cancel to quit.\n\nNote: Location will be saved to Visualizer configuration."),
                QMessageBox::Ok | QMessageBox::Cancel);

        if (result & QMessageBox::Cancel) {
            return false; // exit
        }

        tdriverPath = selectFolder( "Select folder of TDriver default tdriver_parameters.xml file", "Folder", QFileDialog::AcceptOpen, keyLastTDriverDir);
        if (!tdriverPath.endsWith('/')) tdriverPath += '/';
    }
    applicationSettings->setValue( "files/location", tdriverPath );

    // object tree
    collapsedObjectTreeItemPtr = 0;
    expandedObjectTreeItemPtr = 0;
    lastHighlightedObjectKey = 0;

    /* initialize help, context sensitivity needs watch for events */
    installEventFilter( this );

    // create tdriver recorder
    mRecorder = new TDriverRecorder( this );

    // create show xml dialog
    createXMLFileDataWindow();

    createActions();

    // create user interface
    createUi();

    // create find dialog
    createFindDialog();

    // parse parameters xml to retrieve all devices
    if ( !offlineMode ){
        offlineMode = !getXmlParameters( parametersFile );

        if ( !offlineMode && deviceList.count() > 0 ) {
            deviceMenu->setEnabled( true );
            disconnectCurrentSUT->setEnabled( true );
            delayedRefreshAction->setEnabled( true );
            parseSUT->setEnabled( true );
        }
        tabEditor->setParamMap(tdriverXmlParameters);
    }

    // default sut
    setActiveDevice( defaultDevice );

    // update main window title - current sut will be shown
    updateWindowTitle();

    // set visibilities
    clipboardBar->setVisible( clipboardVisible );
    imageViewDock->setVisible( imageVisible );
    propertiesDock->setVisible( propertiesVisible );
#if DEVICE_BUTTONS_ENABLED
    keyboardCommandsDock->setVisible( buttonVisible );
#endif
    shortcutsBar->setVisible( shortcutsVisible );
    appsBar->setVisible( appsVisible );
    editorDock->setVisible( editorVisible );

    // resize window
    resize( windowSize );
    move( windowPosition );

    connectSignals();

    // xml/screen capture output path depending on OS
    outputPath = QDir::tempPath();
#if (defined(Q_OS_WIN32))
    outputPath = QString( getenv( "TEMP" ) ) + "/";
#else
    outputPath = "/tmp/";
#endif

    if ( !offlineMode &&
         !executeTDriverCommand( commandSetOutputPath, "listener set_output_path " + outputPath) ) {
        outputPath = QApplication::applicationDirPath();
    }

    deviceSelected();
    return true;

} // setup


void MainWindow::setActiveDevice(const QString &deviceName )
{
    activeDevice.clear();

    if ( deviceList.contains( deviceName ) ) {
        const QHash<QString, QString> &sut = deviceList.value( deviceName );
        sutName = sut.value( "name" );
        activeDevice["name"] = sutName;
        activeDevice["type"] = sut.value( "type" );
        activeDevice["default_timeout"] = sut.value( "default_timeout" );

        // enable recording menu if if device type is 'kind of' qt
        recordMenu->setEnabled( sut.value( "type" ).contains( "qt", Qt::CaseInsensitive )
                               && !applicationsNamesMap.empty() );
    }
    else {
        sutName.clear();
    }
}


QString MainWindow::getDriverVersionNumber()
{
#if 1
    QString ver(TDriverRubyInterface::globalInstance()->getTDriverVersion());
    qDebug() << FCFL << "got version" << ver;
    return  (ver.isEmpty()) ? "Unknown" : ver;
#else
    QByteArray result = "Unknown";
    BAListMap reply;

    if ( executeTDriverCommand(commandGetVersionNumber, "listener check_version", "", &reply ) ) {
        result = cleanDoneResult(reply["OUTPUT"].first());
    }

    qDebug() << FCFL << "GOT VERSION" << result;
    return QString(result);
#endif
}

QByteArray MainWindow::cleanDoneResult(QByteArray output)
{
    int doneIndex = output.indexOf("done");
    if ( doneIndex > -1 ) {
        // remove done from the end of string
        output.truncate(doneIndex);
    }
    // remove linefeed characters
    return output.trimmed();
}


QString MainWindow::getDeviceType( QString deviceName )
{
    qDebug() << FCFL << deviceName;
    QByteArray result = "Unknown";
    BAListMap reply;
    QString command = deviceName + " get_parameter type";

    if ( executeTDriverCommand( commandGetDeviceType, command, deviceName, &reply ) ) {
        if (reply.contains("parameter")) {
            if (reply.value("parameter").size() == 2 && reply.value("parameter").at(0) == "type") {
                result = reply.value("parameter").at(1);
            }
            else qDebug() << "BAD get_parameter VALUE" << reply.value("parameters");
        }
        else qDebug() << "BAD get_parameter REPLY" << reply;
    }
    else qDebug() << "FAILED get_parameter";

    qDebug() << FCFL << "got" << result;
    return QString(result);
}

QString MainWindow::getDeviceParameter( QString deviceName, QString parameter )
{
    qDebug() << FCFL << deviceName;
    QByteArray result = "Unknown";
    BAListMap reply;
    QString command = deviceName + " get_parameter " + parameter;

    if ( executeTDriverCommand( commandGetDeviceParameter, command, deviceName, &reply ) ) {
        if (reply.contains("parameter")) {
            if (reply.value("parameter").size() == 2 && reply.value("parameter").at(0) == parameter) {
                result = reply.value("parameter").at(1);
            }
            else qDebug() << "BAD get_parameter VALUE" << reply.value("parameters");
        }
        else qDebug() << "BAD get_parameter REPLY" << reply;
    }
    else qDebug() << "FAILED get_parameter";

    qDebug() << FCFL << "got" << result;
    return QString(result);
}


void MainWindow::connectSignals()
{
    connectObjectTreeSignals();
    connectTabWidgetSignals();
    connectImageWidgetSignals();

    QMetaObject::connectSlotsByName( this );
}

// This is called when closing Visualizer. Call threads close method, which
// shuts down thread (closes and then kills it)
void MainWindow::closeEvent( QCloseEvent *event )
{
    if (tabEditor && !tabEditor->mainCloseEvent(event)) {
        qDebug() << "closeEvent rejected by code editor";
        event->ignore();
        return;
    }

    TDriverRubyInterface::globalInstance()->requestClose();

    // close show xml window if still visible
    //if ( xmlView->isVisible() ) { xmlView->close();    }

    // save tdriver path
    applicationSettings->setValue( "files/location", tdriverPath );

    // default window size & position
    applicationSettings->setValue("window/pos", pos());
    applicationSettings->setValue("window/size", size());

    // default sut
    applicationSettings->setValue("sut/activesut", activeDevice.value("name") );
    applicationSettings->setValue("sut/activesuttype", activeDevice.value("type") );

    // object tree settings
    //for ( int i = 0; i < 3 ; i++ ) { applicationSettings->setValue( QString("objecttree/column" + QString::number( i ) ), objectTree->columnWidth( i ) ); }

    // image settings
    applicationSettings->setValue("image/resize", checkBoxResize->isChecked());

    // clipboard contents

    // view visiblity settings
    applicationSettings->setValue( "view/clipboard", clipboardBar->isVisible());
    applicationSettings->setValue( "view/applications", appsBar->isVisible());
    applicationSettings->setValue( "view/image", imageViewDock->isVisible());
    applicationSettings->setValue( "view/properties", propertiesDock->isVisible() );
#if DEVICE_BUTTONS_ENABLED
    applicationSettings->setValue( "view/buttons", viewButtons->isChecked() );
#else
    applicationSettings->remove("view/buttons");
#endif
    applicationSettings->setValue( "view/shortcuts", shortcutsBar->isVisible());
    applicationSettings->setValue( "view/editor", editorDock->isVisible());

    applicationSettings->setValue( "font/settings", defaultFont->toString() );
}

// Event filter, catches F1/HELP key events and processes them
bool MainWindow::eventFilter(QObject * object, QEvent *event) {

    Q_UNUSED( object );

    if (event->type() == QEvent::KeyPress) {

        QKeyEvent *ke = static_cast<QKeyEvent *>(event);

        //qDebug() << "modifiers(): " << ke->modifiers() << ", key(): " << ke->key();

        if (ke->key() == Qt::Key_F1 || ke->key() == Qt::Key_Help) {

            //QWidget *widget = 0;
            QString page = "qdoc-temp/index.html";
#if 0
            // Context sensitivity disabled for now
            // You need to also remove the line "visualizerAssistant->setShortcut(tr("F1"));"
            // from the createTopMenuBar method to enable processing of F1 ket events in this event handler
            if (object->isWidgetType()) {

                widget = static_cast<QWidget *>(object)-> focusWidget();
                if (widget == objectTree)
                    page = "tree.html";
                }else {
                    page = "devices.html";
                }
#endif
            showContextVisualizerAssistant(page);

            return true;
        }
    }
    return QMainWindow::eventFilter(object, event);
}


// MainWindow listener for keypresses
void MainWindow::keyPressEvent ( QKeyEvent * event )
{
    // qDebug() << "MainWindow::keyPressEvent: " << event->key();
    if ( QApplication::focusWidget() == objectTree && objectTree->currentItem() != NULL )
        objectTreeKeyPressEvent( event );
    else
        event->ignore();

    collapsedObjectTreeItemPtr = 0;
    expandedObjectTreeItemPtr = 0;
}


bool MainWindow::isDeviceSelected()
{
    return !activeDevice.isEmpty();
}


void MainWindow::noDeviceSelectedPopup()
{
    if ( !offlineMode ){
        QMessageBox::critical(0, tr( "Error" ), "Unable to refresh due to no device selected.\n\nPlease select one from devices menu." );
    }
}


QString MainWindow::selectFolder(QString title, QString filter, QFileDialog::AcceptMode mode, const QString &saveDirKey)
{
    QFileDialog dialog( this );

    dialog.setAcceptMode( mode );
    dialog.setFileMode( QFileDialog::Directory );
    dialog.setNameFilter( filter );
    dialog.setWindowTitle( title );
    dialog.setViewMode( QFileDialog::List );
    if (!saveDirKey.isEmpty()) {
        dialog.setDirectory(applicationSettings->value(saveDirKey, QVariant("")).toString());
    }

    QString ret;
    if (dialog.exec()) {
        QString dirName = dialog.selectedFiles().at(0);
        if (!dirName.isEmpty()) {
            if (!saveDirKey.isEmpty()) {
                applicationSettings->setValue(saveDirKey, dirName);
            }
            ret = dirName;
        }
    }
    return ret;
}


void MainWindow::statusbar( QString text, int timeout )
{
    statusBar()->showMessage( text, timeout );
    statusBar()->repaint();
    qApp->processEvents();
}


void MainWindow::statusbar( QString text, int currentProgressValue, int maxProgressValue, int timeout )
{
    int progress = int( (  float( currentProgressValue ) / float( maxProgressValue ) ) * float( 100 ) );
    statusBar()->showMessage( text + " " + QString::number( progress ) + "%", timeout );
    statusBar()->repaint();
}

void MainWindow::handleRbiError(QString title, QString text, QString details)
{
    tdriverMsgAppend(tr("Error from TDriver interface:\n")
                     + title
                     + "\n____________________\n"
                     + text
                     + "\n____________________\n"
                     + details);
}


void MainWindow::processErrorMessage(ExecuteCommandType commandType, const BAListMap &msg, const QString &additionalInformation,
                                     unsigned &resultEnum, QString &clearError, QString &shortError, QString &fullError )
{
    QStringList errList;
    foreach(const QByteArray &ba, msg.value("error")) {
        errList << QString(ba);
    }
    QString tdriverError = errList.join("\n");

    QStringList exList;
    foreach(const QByteArray &ba, msg.value("exception")) {
        exList << QString(ba);
    }

    // do clearError
    {
        switch ( commandType ) {
        case MainWindow::commandListApps: clearError = tr("Error retrieving applications list."); break;
        case MainWindow::commandClassMethods: clearError = tr("Error retrieving methods list for %1.").arg(additionalInformation); break;
        case MainWindow::commandSignalList: clearError = tr("Error retrieving signal list for %1.").arg(additionalInformation); break;
        case MainWindow::commandDisconnectSUT: clearError = tr("Error disconnecting SUT %1.").arg(additionalInformation); break;
        case MainWindow::commandTapScreen: clearError = tr("Error performing tap to screen."); break;
        case MainWindow::commandRefreshUI: clearError = tr("Failed to refresh UI data."); break;
        case MainWindow::commandRefreshImage: clearError = tr("Failed to refresh screen capture image."); break;
        case MainWindow::commandKeyPress: clearError = tr("Failed to press key %1.").arg(additionalInformation); break;
        case MainWindow::commandSetAttribute: clearError = tr("Failed to set attribute %1.").arg(additionalInformation); break;
        case MainWindow::commandGetDeviceType: clearError = tr("Failed to get device type for %1.").arg(additionalInformation); break;
        case MainWindow::commandGetVersionNumber: clearError = tr("Failed to retrieve TDriver version number."); break;
        default: clearError = tr("Error with unknown command");
        }
    }

    // do shortError
    {
        if ( commandType == commandCheckApiFixture) {
            resultEnum = SILENT;
        }

        else if ( tdriverError.contains( "No plugins and no ui for server" ) ) {
            shortError = tr( "Failed to refresh application screen capture.\n\nLaunch some application with UI and try again." );
            resultEnum = WARNING;
        }

        else if ( tdriverError.contains( "No connection could be made because the target machine actively refused it." ) ) {
            shortError = tr( "Please start/restart QTTAS server." );
            resultEnum = DISCONNECT | RETRY;
        }

        else if( tdriverError.contains( "An existing connection was forcibly closed by the remote host." ) ) {
            shortError = tr( "Please disconnect the SUT from file menu and try again.\n\nIf the problem persists, restart QTTAS server/device or contact support." );
            resultEnum = DISCONNECT | RETRY;
        }

        else if( tdriverError.contains( "Connection refused" ) ) {
            shortError = tr( "Unable to connect to target. Please verify that required servers are running and target is connected properly.\n\nIf the problem persists, contact support." );
            resultEnum = DISCONNECT | RETRY;
        }

        else if( tdriverError.contains( "No data retrieved (IOError)" ) ) {
            shortError = tr( "Unable to read data from target. Please verify that required servers are running and target is connected properly.\n\nIf the problem persists, contact support." );
            resultEnum = DISCONNECT | RETRY;
        }

        else if( tdriverError.contains( "Broken pipe (Errno::EPIPE)" ) ) {
            shortError = tr( "Unable to connect to target due to broken pipe.\n\nPlease disconnect SUT, verify that required servers are running/target is connected properly and try again.\n\nIf the problem persists, contact support." );
            resultEnum = DISCONNECT | RETRY;
        }

        else {
            // unknown error
            shortError = tdriverError;
            resultEnum = FAIL;
        }
    }

    // do fullError
    {
        fullError = tr("%1\n\n%2").arg(clearError).arg(shortError);
        if (shortError != tdriverError) {
            fullError += "\n(original: " + tdriverError + ")";
        }
        if (!exList.empty()) {
            while (exList.size() < 2) exList << QString();
            fullError += tr("\n\nException '%1':\n%2\n\nBacktrace:\n%3").
                         arg(exList.takeFirst()).
                         arg(exList.takeFirst()).
                         arg(exList.join("\n"));
            fullError.replace(":in `", ":\n  in `");
        }
    }

}


bool MainWindow::executeTDriverCommand( ExecuteCommandType commandType, const QString &commandString, const QString &additionalInformation, BAListMap *reply )
{
    QString clearError;
    QString shortError;
    QString fullError;
    bool exit = false;
    bool result = true;
    int iteration = 0;
    int default_timeout=activeDevice["default_timeout"].toInt()*1000;
    unsigned resultEnum = OK;

    if (default_timeout==0){
        default_timeout=35000;
    }

    do {
        BAListMap msg;
        msg["input"] = commandString.toAscii().split(' ');

        qDebug() << FCFL << "going to execute" << msg << "Using timeout: " << default_timeout;
        QTime t;
        t.start();
        /*bool response1 =*/
        TDriverRubyInterface::globalInstance()->executeCmd("visualization", msg, default_timeout );
        if (msg.contains("error")) {
            qDebug() << FCFL << "failure time" << float(t.elapsed())/1000.0 << "reply" << msg;

            result = false;
            exit = true;
            processErrorMessage(commandType, msg, additionalInformation,
                                resultEnum, clearError, shortError, fullError);

            if ( resultEnum & FAIL || iteration > 0 ) {
                // exit if failed again or no retries allowed..
                exit = true;
            }

            if ( resultEnum & DISCONNECT ) {
                // disconnect
                msg.clear();
                msg["input"] << activeDevice.value( "name" ).toAscii() << "disconnect";
                /*bool response2 =*/
                TDriverRubyInterface::globalInstance()->executeCmd("visualization", msg, default_timeout );
                if (msg.contains("error")) {
                    fullError += "\n\nDisconnect after error failed!";
                    result = false;
                    exit = true;
                }
                else {
                    fullError += "\n\nDisconnect after error succeeded.";
                    // disconnect passed -- retry
                    currentApplication.clear();
                    result = false;
                    if (!(resultEnum & FAIL)) exit = false;
                }
            }
        }
        else {
            qDebug() << FCFL << "success time" << float(t.elapsed())/1000.0 << "reply" << msg;
            if (reply) {
                *reply = msg;
            }
            result = true;
            exit = true;
        }

        iteration++;
    } while (!exit);

    if ( !result && !(resultEnum & SILENT) ) {
        tdriverMsgAppend(fullError);
    }

    return result;
}


void MainWindow::createActions()
{

    parseSUT = new QAction( tr("&Parse TDriver parameters xml file..." ), this);
    parseSUT->setObjectName("main parsesut");
    parseSUT->setShortcuts(QList<QKeySequence>() <<
                           QKeySequence(tr("Ctrl+M, P")) <<
                           QKeySequence(tr("Ctrl+M, Ctrl+P")));
    parseSUT->setDisabled( true );

    connect( parseSUT, SIGNAL( triggered() ), this, SLOT(getParameterXML()));

    loadXmlAction = new QAction( this );
    loadXmlAction->setObjectName("main loadxml");
    loadXmlAction->setText( "&Load state XML file" );
    loadXmlAction->setShortcuts(QList<QKeySequence>() <<
                                QKeySequence(tr("Ctrl+M, L")) <<
                                QKeySequence(tr("Ctrl+M, Ctrl+L")));

    connect( loadXmlAction, SIGNAL( triggered() ), this, SLOT(loadFileData()));


    saveStateAction = new QAction( this );
    saveStateAction->setObjectName("main savestate");
    saveStateAction->setText( "&Save current state to folder..." );
    saveStateAction->setShortcuts(QList<QKeySequence>() <<
                                  QKeySequence(tr("Ctrl+M, S")) <<
                                  QKeySequence(tr("Ctrl+M, Ctrl+S")));

    connect( saveStateAction, SIGNAL( triggered() ), this, SLOT(saveStateAsArchive()));

    fontAction = new QAction(tr( "Select default f&ont..." ), this);
    fontAction->setObjectName("main font");
    //fontAction->setShortcut( QKeySequence( Qt::ControlModifier + Qt::Key_T ) );

    connect( fontAction, SIGNAL( triggered() ), this, SLOT(openFontDialog()));

    refreshAction = new QAction(tr("&Refresh"), this);
    refreshAction->setObjectName("main refresh");
    refreshAction->setShortcut(QKeySequence(tr("Ctrl+R")));
    // note: QKeySequence(QKeySequence::Refresh) is F5 in some platforms, Ctrl+R in others
    //refreshAction->setDisabled( true );

    connect( refreshAction, SIGNAL( triggered() ), this, SLOT(forceRefreshData()));

    delayedRefreshAction = new QAction(tr("Refresh in 5 secs"), this );
    delayedRefreshAction->setObjectName("main delayed delayedRefresh");
    delayedRefreshAction->setShortcut(QKeySequence(tr("Ctrl+Alt+R")));
    //delayedRefreshAction->setDisabled( true );

    connect( delayedRefreshAction, SIGNAL(triggered()), this, SLOT(delayedRefreshData()));

    disconnectCurrentSUT = new QAction( tr( "Dis&connect SUT" ), this );
    disconnectCurrentSUT->setObjectName("main disconnectsut");
    disconnectCurrentSUT->setShortcuts(QList<QKeySequence>() <<
                                       QKeySequence(tr("Ctrl+M, C")) <<
                                       QKeySequence(tr("Ctrl+M, Ctrl+C")));
    disconnectCurrentSUT->setDisabled( true );

    connect( disconnectCurrentSUT, SIGNAL( triggered() ), this, SLOT(disconnectSUT()));

    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setObjectName("main exit");
    exitAction->setShortcut( QKeySequence( tr("Alt+X")));

    connect( exitAction, SIGNAL( triggered() ), this, SLOT(close()));

    findAction = new QAction(tr("&Find"), this);
    findAction->setObjectName("main find");
    //findAction->setShortcut( QKeySequence(tr("Ctrl+F")));

    connect( findAction, SIGNAL( triggered() ), this, SLOT( showFindDialog() ) );

    viewExpandAll = new QAction( this );
    viewExpandAll->setObjectName("main tree expand");
    viewExpandAll->setText( "&Expand all" );
    viewExpandAll->setShortcut( QKeySequence( Qt::ControlModifier + Qt::Key_Right ) );
    viewExpandAll->setCheckable( false );

    connect( viewExpandAll, SIGNAL( triggered() ), this, SLOT( objectTreeExpandAll() ) );

    viewCollapseAll = new QAction( this );
    viewCollapseAll->setObjectName("main tree collapse");
    viewCollapseAll->setText( "&Collapse all" );
    viewCollapseAll->setShortcut( QKeySequence( Qt::ControlModifier + Qt::Key_Left ) );
    viewCollapseAll->setCheckable( false );

    connect( viewCollapseAll, SIGNAL( triggered() ), this, SLOT( objectTreeCollapseAll() ) );

    showXmlAction = new QAction( this );
    showXmlAction->setObjectName("main showxml");
    showXmlAction->setText( tr( "&Show source XML" ) );
    showXmlAction->setShortcut( QKeySequence( Qt::ControlModifier + Qt::Key_U ) );

    connect( showXmlAction, SIGNAL( triggered() ), this, SLOT( showXMLDialog() ) );

    recordAction = new QAction( this );
    recordAction->setObjectName("main record");
    recordAction->setText( tr( "&Open Recorder Dialog" ) );
    recordAction->setShortcut( tr( "F6" ) );

    connect( recordAction, SIGNAL( triggered() ), this, SLOT( openRecordWindow() ) );

    visualizerAssistant = new QAction( this );
    visualizerAssistant->setObjectName("main help assistant");
    visualizerAssistant->setText( tr( "TDriver &Help" ) );
    // Remove the next line if F1 presses are to be handled in the custom event handler ( for context sensitivity ).
    visualizerAssistant->setShortcut( tr( "F1" ) );

    connect( visualizerAssistant, SIGNAL( triggered() ), this, SLOT( showMainVisualizerAssistant() ) );

    visualizerHelp = new QAction( this );
    visualizerHelp->setObjectName("main help visualizer");
    visualizerHelp->setText( tr( "&Visualizer Help" ) );

    connect( visualizerHelp, SIGNAL( triggered() ), this, SLOT( showVisualizerHelp() ) );

    aboutVisualizer = new QAction(this);
    aboutVisualizer->setObjectName("main help about");
    aboutVisualizer->setText( tr( "About Visualizer" ));

    connect( aboutVisualizer, SIGNAL( triggered() ), this, SLOT( showAboutVisualizer() ) );

}


void MainWindow::createAppsBar()
{
    appsBar = new QToolBar( tr( "Applications" ));
    appsBar->setObjectName("apps");
    //appsBar->addAction(exitAction);

    addToolBar(Qt::LeftToolBarArea, appsBar);
}


void MainWindow::createShortcutsBar()
{
    shortcutsBar = new QToolBar( tr( "Shortcuts" ));
    shortcutsBar->setObjectName("shortcuts");

    shortcutsBar->addAction(refreshAction);
    shortcutsBar->addSeparator();
    shortcutsBar->addAction(delayedRefreshAction);
    shortcutsBar->addSeparator();
    shortcutsBar->addAction(showXmlAction);
    shortcutsBar->addSeparator();
    shortcutsBar->addAction(loadXmlAction);

    //shortcutsBar->addAction(exitAction);

    addToolBar(shortcutsBar);
}

void MainWindow::createClipboardBar()
{

    clipboardBar = new QToolBar( tr("Clipboard contents"));
    clipboardBar->setObjectName("clipboard");

    //clipboardBar->setFloating( false );
    clipboardBar->setVisible( true );

    objectAttributeLineEdit = new QLineEdit;
    objectAttributeLineEdit->setObjectName("clipboard");

    clipboardBar->addWidget( new QLabel(tr("Clipboard: ")) );
    clipboardBar->addWidget( objectAttributeLineEdit );

    addToolBar(Qt::TopToolBarArea, clipboardBar);
}

void MainWindow::updateClipboardText( QString text, bool appendText ) {

    QClipboard *clipboard = QApplication::clipboard();

    if ( appendText ) { text.prepend( clipboard->text().append(".") ); }

    clipboard->setText( text );
    objectAttributeLineEdit->setText( text );

}

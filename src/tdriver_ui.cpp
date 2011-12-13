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

#include "../common/version.h"

#include "tdriver_tabbededitor.h"
#include "tdriver_featureditor.h"

#include <QUrl>
#include <QScrollArea>
#include <QToolBar>

#include "tdriver_debug_macros.h"

void MainWindow::setupTableWidgetHeader( QString headers, QTableWidget * table)
{

    QStringList tableHeader = headers.split("|");
    table->setColumnCount( tableHeader.count() );

    for( int i = 0; i < tableHeader.count(); i++ ) {
        QTableWidgetItem *item = new QTableWidgetItem( tableHeader.at(i) );
        //item->setFont( *defaultFont );
        table->setHorizontalHeaderItem( i, item );
    }

}

void MainWindow::showAboutVisualizer() {

    QMessageBox::about(
            this,
            tr("About Visualizer"),
            tr("<table><tr><td width=\"250\" align=\"center\" valign=\"middle\">") +
            tr("TDriver Visualizer") +
            tr("<br>Build number: <b>") + VISUALIZER_VERSION + tr("</b>") +
            tr("</tr></td></table>")
            );

}

//    Shows the Visualizer help page.
void MainWindow::showVisualizerHelp()
{
    //showContextVisualizerAssistant("howto-visualizer.html");
    //QUrl helpUrl("https://cwiki.nokia.com/Testability/TDriverVisualizer");

    QUrl helpUrl("https://projects.forum.nokia.com/Testabilitydriver/wiki/FeatureVisualizer");
    QDesktopServices::openUrl(helpUrl);
}


//    Shows TDriver API Doc for qt windows
void MainWindow::showVisualizerQtWindowsHelp()
{
    //showContextVisualizerAssistant("howto-visualizer.html");
    //QUrl helpUrl("https://cwiki.nokia.com/Testability/TDriverVisualizer");
    QUrl helpUrl(TDriverUtil::helpUrlString("/qt_windows/index.html"));
    QDesktopServices::openUrl(helpUrl);
}

//    Shows TDriver API Doc for qt linux
void MainWindow::showVisualizerQtLinuxHelp()
{
    //showContextVisualizerAssistant("howto-visualizer.html");
    //QUrl helpUrl("https://cwiki.nokia.com/Testability/TDriverVisualizer");
    QUrl helpUrl(TDriverUtil::helpUrlString("/qt_linux/index.html"));
    QDesktopServices::openUrl(helpUrl);
}

//    Shows TDriver API Doc for qt symbian
void MainWindow::showVisualizerQtSymbianHelp()
{
    //showContextVisualizerAssistant("howto-visualizer.html");
    //QUrl helpUrl("https://cwiki.nokia.com/Testability/TDriverVisualizer");
    QUrl helpUrl(TDriverUtil::helpUrlString("/qt_symbian/index.html"));
    QDesktopServices::openUrl(helpUrl);
}

//    Shows the main TDriver Visualizer help page.
void MainWindow::showMainVisualizerAssistant()
{
    QUrl helpUrl("https://projects.forum.nokia.com/Testabilitydriver/wiki/FeatureDocumentation");
    QDesktopServices::openUrl(helpUrl);
}

//    Shows the defined help page
void MainWindow::showContextVisualizerAssistant(const QString& page)
{

    Q_UNUSED( page );

    //QStringList argList;
    //argList << "/C" << "help\\index.html";

    //QProcess* helpProc = new QProcess(this);
    //helpProc->setEnvironment( QProcess::systemEnvironment());
    //helpProc->start("cmd.exe", argList);

    QUrl helpUrl(TDriverUtil::helpUrlString("index.html"));
    QDesktopServices::openUrl(helpUrl);

}

void MainWindow::createUi()
{
    setDockOptions(AnimatedDocks|AllowNestedDocks);
    createImageViewDockWidget();
    createTreeViewDockWidget();
    createPropertiesDockWidget();
    createTopMenuBar();
    createAppsBar();
    createShortcutsBar();
    createClipboardBar();
    createEditorDocks();
    createFeaturEditorDocks();

    connect(featurEditor, SIGNAL(fileEditRequest(QString)),
            tabEditor, SLOT(gotoLine(QString)));

#if DEVICE_BUTTONS_ENABLED
    createKeyboardCommands();
#endif

    // layout of main window: add objecttree to central and set it, add menubar as menu
    statusBar()->setObjectName("main");
    setCentralWidget( objectTree );
    setMenuBar( menubar );

    updateWindowTitle();

    tabWidget->setCurrentIndex( 0 );

    statusBar()->show();
    statusBar()->clearMessage();

    createFindDialogShortcuts();
}


void MainWindow::updateWindowTitle() {

    QString tempTitle = tr("TDriver Visualizer v%1 - ").arg(VISUALIZER_VERSION);

    if ( currentApplication.isForeground() ){
        tempTitle += tr("Foreground app: ");
    }
    tempTitle += currentApplication.name + " - ";

    if (!currentApplication.id.isEmpty()) {
        tempTitle += "(" + currentApplication.id + ") - ";
    }

    if (!titleFileText.isEmpty()) {
        tempTitle += "(" + titleFileText + ") - ";
    }

    tempTitle += ( offlineMode ) ?
                 tr("Offline mode") : ( activeDevice.isEmpty() ?
                                        tr("no device selected") : activeDevice);
    qDebug() << FCFL << tempTitle;
    setWindowTitle( tempTitle );
}



void MainWindow::changeImageResize(bool resize)
{
    imageScroller->setWidgetResizable(resize);
    imageWidget->changeImageResized(resize);
    imageWidget->update();
}


void MainWindow::changeImageLeftClick(int index) {

    //qDebug() << __FUNCTION__ << index << "=" << imageLeftClickChooser->itemText(index) << " / " << imageLeftClickChooser->itemData(index);
    imageWidget->setLeftClickAction(imageLeftClickChooser->itemData(index).toInt());

}


void MainWindow::createTreeViewDockWidget()
{

    objectTree = new QTreeWidget();
    objectTree->setObjectName("tree");

    //    objectTree->header()->setStretchLastSection(false);
    //    objectTree->header()->setResizeMode( QHeaderView::Stretch );

    objectTree->header()->setStretchLastSection( true );
    objectTree->header()->setResizeMode( QHeaderView::Interactive );

    objectTree->setColumnCount( 3 );

    for ( int i = 0; i < 3; i++ ) {

        objectTree->setColumnWidth( i, 250 );

        //objectTree->setColumnWidth( i, QSettings().value( QString( "objecttree/column" + QString::number( i ) ), 350 ).toInt() );
    }

    QStringList labels;
    labels << " type " << " name " << " id ";
    objectTree->setHeaderLabels ( labels );

}

// create properties dock widget
void MainWindow::createPropertiesDockWidget() {

    propertiesDock = new QDockWidget(tr(" Properties "), this);
    propertiesDock->setObjectName("properties");

    propertiesDock->setFeatures( DOCK_FEATURES_DEFAULT );

    tabWidget = new QTabWidget();
    tabWidget->setObjectName("properties");

    createPropertiesDockWidgetPropertiesTabWidget();
    createPropertiesDockWidgetMethodsTabWidget();
    createPropertiesDockWidgetSignalsTabWidget();
    createPropertiesDockWidgetApiTabWidget();

    tabWidget->setTabText( tabWidget->indexOf( propertiesTab ), QApplication::translate( "MainWindow", "A&ttributes", 0, QApplication::UnicodeUTF8 ) );
    tabWidget->setTabText( tabWidget->indexOf( methodsTab ), QApplication::translate( "MainWindow", "&Methods", 0, QApplication::UnicodeUTF8 ) );
    tabWidget->setTabText( tabWidget->indexOf( signalsTab ), QApplication::translate( "MainWindow", "&Signals", 0, QApplication::UnicodeUTF8 ) );
#if !DISABLE_API_TAB_PENDING_REMOVAL
    tabWidget->setTabText( tabWidget->indexOf( apiTab ), QApplication::translate( "MainWindow", "AP&I", 0, QApplication::UnicodeUTF8 ) );
#endif

    tabWidget->setCurrentIndex( 0 );

    propertiesDock->setWidget(tabWidget);

}

void MainWindow::createPropertiesDockWidgetPropertiesTabWidget() {

    // properties
    propertiesTab = new QWidget();
    propertiesTab->setObjectName("properties attributes");

    propertiesLayout = new QStackedLayout(propertiesTab);
    propertiesLayout->setObjectName("properties attributes");
    propertiesTab->setLayout(propertiesLayout);

    propertiesTable = new QTableWidget(propertiesTab);
    propertiesTable->setObjectName("properties attributes");

    propertiesLayout->addWidget(propertiesTable);
    propertiesTable->clear();
    propertiesTable->setColumnCount(2);
    //    propertiesTable->setFont( *defaultFont );
    setupTableWidgetHeader( "Name|Value", propertiesTable );

    tabWidget->addTab(propertiesTab, QString());

}

void MainWindow::createPropertiesDockWidgetMethodsTabWidget() {

    // methods
    methodsTab = new QWidget();
    methodsTab->setObjectName("properties methods");

    methodsLayout = new QStackedLayout(methodsTab);
    methodsLayout->setObjectName("properties methods");
    methodsTab->setLayout(methodsLayout);

    methodsTable = new QTableWidget(methodsTab);
    methodsTable->setObjectName("properties methods");

    methodsLayout->addWidget(methodsTable);

    methodsTable->clear();
    methodsTable->setColumnCount( 2 );
    setupTableWidgetHeader( "Name|Example", methodsTable );

    tabWidget->addTab(methodsTab, QString());

    // disable methods tab when running in Offline mode
    if ( offlineMode ){
        tabWidget->setTabEnabled( tabWidget->indexOf( methodsTab ), false );
    }

}
void MainWindow::createPropertiesDockWidgetSignalsTabWidget() {

    // signals
    signalsTab = new QWidget();
    signalsTab->setObjectName("properties signals");

    signalsLayout = new QStackedLayout(signalsTab);
    signalsLayout->setObjectName("properties signals");
    signalsTab->setLayout(signalsLayout);

    signalsTable = new QTableWidget(signalsTab);
    signalsTable->setObjectName("properties signals");

    signalsLayout->addWidget(signalsTable);

    signalsTable->clear();
    signalsTable->setColumnCount( 1 );
    setupTableWidgetHeader( "Name", signalsTable );

    tabWidget->addTab(signalsTab, QString());

    // disable signals tab when running in Offline mode
    if ( offlineMode ){
        tabWidget->setTabEnabled( tabWidget->indexOf( signalsTab ), false );
    }

}

void MainWindow::createPropertiesDockWidgetApiTabWidget()
{
#if !DISABLE_API_TAB_PENDING_REMOVAL
    // Object API tab
    apiTab = new QWidget();
    apiTab->setObjectName("properties api");

    apiLayout = new QStackedLayout(apiTab);
    apiLayout->setObjectName("properties api");
    apiTab->setLayout(apiLayout);

    apiTable = new QTableWidget(apiTab);
    apiTable->setObjectName("properties api");
    //apiTable->setSortingEnabled( true );
    apiLayout->addWidget(apiTable);

    apiTable->clear();
    apiTable->setColumnCount( 3 );
    setupTableWidgetHeader( "Returns|Method|Arguments", apiTable );
    tabWidget->addTab(apiTab, QString());
#endif
}


void MainWindow::createImageViewDockWidget()
{
    // image resize setting
    bool resizeSetting = QSettings().value( "image/resize", true ).toBool();

    // Add imagewidged and resize checkbox
    imageViewDock = new QDockWidget( tr(" Image View "), this );
    imageViewDock->setObjectName("imageview");

    imageViewDock->setFeatures( DOCK_FEATURES_DEFAULT );

    QGroupBox * imageViewBox = new QGroupBox();
    imageViewBox->setObjectName("imageview");

    QVBoxLayout * layout = new QVBoxLayout;
    layout->setObjectName("imageview");

    // Image view
    imageWidget = new TDriverImageView( this, this );
    imageWidget->setObjectName("imageview");
    connect(imageWidget, SIGNAL(statusBarMessage(QString,int)), this, SLOT(statusbar(QString,int)));

    imageScroller = new QScrollArea;
    imageScroller->setObjectName("imagescroller");
    imageScroller->setAlignment(Qt::AlignCenter);
    imageScroller->setWidget(imageWidget);
    layout->addWidget( imageScroller, 1);

    // image
    {
        QToolBar *container = new QToolBar();
        container->setOrientation(Qt::Vertical);
        container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        checkBoxResize = new QCheckBox( tr( "Resi&ze image " ), this );
        checkBoxResize->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        checkBoxResize->setChecked(resizeSetting);
        checkBoxResize->setObjectName("imageview resize");
        container->addWidget(checkBoxResize);

        imageLeftClickChooser = new QComboBox();
        imageLeftClickChooser->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        imageLeftClickChooser->addItem(tr("Left click inspects (no hover)"), TDriverImageView::VISUALIZER_INSPECT);
        imageLeftClickChooser->addItem(tr("Left click sends tap to SUT"), TDriverImageView::SUT_DEFAULT_TAP);
        imageLeftClickChooser->setCurrentIndex(1);
        imageLeftClickChooser->addItem(tr("Left click inserts coordines"), TDriverImageView::ED_COORD_INSERT);
        imageLeftClickChooser->addItem(tr("Left click inserts object tap"), TDriverImageView::ED_TESTOBJ_INSERT);
        container->addWidget(imageLeftClickChooser);

        layout->addWidget(container);
    }

    imageWidget->changeImageResized(resizeSetting);
    imageScroller->setWidgetResizable(resizeSetting);
    connect(checkBoxResize, SIGNAL(toggled(bool)), this, SLOT(changeImageResize(bool)));

    connect(imageLeftClickChooser, SIGNAL(activated(int)), this, SLOT( changeImageLeftClick(int) ) );

    imageViewBox->setLayout( layout );
    imageViewDock->setWidget( imageViewBox );


}


MainWindow::ContextMenuSelection MainWindow::showCopyAppendContextMenu()
{
    // display context menu
    QMenu contextMenu(this);

    QAction *copy = contextMenu.addAction( tr( "&Copy to clipoard" ) );
    copy->setObjectName("context copy");
    QAction *copyPath = contextMenu.addAction( tr( "&Copy to clipoard with path" ) );
    copyPath->setObjectName("context copypath");

    QAction *append = contextMenu.addAction( tr( "&Append to clipoard" ) );
    append->setObjectName("context append");
    QAction *appendPath = contextMenu.addAction( tr( "&Append to clipoard with path" ) );
    appendPath->setObjectName("context appendpath");

    QAction *insert = NULL;
    if(tabEditor->currentHasWritableCursor()) {
        insert = contextMenu.addAction( tr( "&Insert to editor" ) );
        insert->setObjectName("context insert");
    }
    QAction *insertPath = NULL;
    if(tabEditor->currentHasWritableCursor()) {
        insertPath = contextMenu.addAction( tr( "&Insert to editor with path" ) );
        insertPath->setObjectName("context insertpath");
    }

    ContextMenuSelection result = cancelAction;

    //position menu according to cursor position
    QAction *selectedAction = contextMenu.exec( QCursor::pos() );

    if (selectedAction) {
        if ( selectedAction == copy )
            result = copyAction;
        else if ( selectedAction == append )
            result = appendAction;
        else if ( selectedAction == insert )
            result = insertAction;
        else if ( selectedAction == copyPath )
            result = copyPathAction;
        else if ( selectedAction == appendPath )
            result = appendPathAction;
        else if ( selectedAction == insertPath )
            result = insertPathAction;
        else
            qWarning("Bad context menu action in %s:%s", __FILE__, __FUNCTION__);

    }
    return result;
}




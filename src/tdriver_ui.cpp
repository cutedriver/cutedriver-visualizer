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

#include "../common/version.h"

#include <tdriver_tabbededitor.h>

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
    //QDesktopServices::openUrl(helpUrl);
    QString pagePath;
    pagePath=QApplication::applicationDirPath();
    pagePath += "/help/howto-use-visualizer.html";
    QUrl helpUrl(pagePath);
    QDesktopServices::openUrl(helpUrl);

}

//    Shows the main TDriver Visualizer help page.
void MainWindow::showMainVisualizerAssistant()
{
    QString pagePath;
    pagePath=QApplication::applicationDirPath();
    pagePath += "/help/index.html";
    QUrl helpUrl(pagePath);
    QDesktopServices::openUrl(helpUrl);
}

//    Shows the defined help page using Assistant.
void MainWindow::showContextVisualizerAssistant(const QString& page)
{

    Q_UNUSED( page );

    //QStringList argList;
    //argList << "/C" << "help\\index.html";

    //QProcess* helpProc = new QProcess(this);
    //helpProc->setEnvironment( QProcess::systemEnvironment());
    //helpProc->start("cmd.exe", argList);

    QString pagePath(QDir::currentPath());
    pagePath += "/help/index.html";
    QUrl helpUrl(pagePath);
    QDesktopServices::openUrl(helpUrl);

}

void MainWindow::createUi()
{

    createGridLayout();

    createImageViewDockWidget();
    createTreeViewDockWidget();
    createPropertiesDockWidget();
    createClipboardDock();
    createShortcuts();
    createEditorDocks();
    createKeyboardCommands();

    createTopMenuBar();

    // layout of main window: add objecttree to central and set it, add menubar as menu
    gridLayout->addWidget( objectTree );
    statusBar()->setObjectName("main");
    setCentralWidget( objectTree );
    setMenuBar( menubar );
    gridLayoutWidget->setLayout( gridLayout );

    updateWindowTitle();

    tabWidget->setCurrentIndex( 0 );

    statusBar()->show();
    statusBar()->clearMessage();

}

// create it as a gridLayOutWidget
void MainWindow::createGridLayout()
{
    gridLayoutWidget = new QWidget( this );
    gridLayoutWidget->setObjectName("main tree");

    gridLayout = new QVBoxLayout( gridLayoutWidget );
    gridLayout->setObjectName("main tree");
}




void MainWindow::updateWindowTitle() {

    QString tempTitle =
            tr("TDriver Visualizer v") + VISUALIZER_VERSION +
            tr(" - ");

    if ( foregroundApplication && !applicationsHash.isEmpty() ){
        tempTitle += tr("Foreground app. - ");
    }
    else {
        tempTitle +=  ( currentApplication.value( "name" ).isEmpty() ? "" : currentApplication.value( "name" ) + " (" + currentApplication.value( "id" ) + ") - " );
    }

    tempTitle += ( offlineMode ) ?
                 tr("Offline mode") : ( activeDevice.isEmpty() ?
                                        tr("no device selected") : activeDevice.value("name") );
    setWindowTitle( tempTitle );
}

void MainWindow::visiblityChangedClipboard( bool state ){

    viewClipboard->setChecked( state );

}

void MainWindow::visiblityChangedImage( bool state ){

    viewImage->setChecked( state );

}

void MainWindow::visiblityChangedProperties( bool state ){

    viewProperties->setChecked( state );

}

void MainWindow::visiblityChangedShortcuts( bool state ){

    viewShortcuts->setChecked( state );

}

void MainWindow::visiblityChangedEditor( bool state ){

    viewEditor->setChecked( state );

}

void MainWindow::visiblityChangedButtons( bool state ){

    viewButtons->setChecked( state );

}

void MainWindow::changeClipboardVisibility()
{
    clipboardDock->setVisible( ( clipboardDock->isVisible() ? false : true )  );
}

void MainWindow::changeShortcutsVisibility() {

    shortcutsDock->setVisible( viewShortcuts->isChecked() );

}

void MainWindow::changeEditorVisibility() {

    editorDock->setVisible( viewEditor->isChecked() );

}

void MainWindow::changePropertiesVisibility() {

    tabWidget->setVisible( viewProperties->isChecked()  );

}

void MainWindow::changeKeyboardCommandsVisibility() {

    keyboardCommandsDock->setVisible( viewButtons->isChecked() );

}

void MainWindow::changeImagesVisibility() {

    imageViewDock->setVisible( viewImage->isChecked() );

}


void MainWindow::changeImageResize() {

    imageWidget->changeImageResized(checkBoxResize->checkState());
    imageWidget->update();

}


void MainWindow::changeImageLeftClick(int index) {

    //qDebug() << __FUNCTION__ << index << "=" << imageLeftClickChooser->itemText(index) << " / " << imageLeftClickChooser->itemData(index);
    imageWidget->setLeftClickAction(imageLeftClickChooser->itemData(index).toInt());

}


void MainWindow::createTreeViewDockWidget()
{

    objectTree = new QTreeWidget( gridLayoutWidget );
    objectTree->setObjectName("tree");

    //    objectTree->header()->setStretchLastSection(false);
    //    objectTree->header()->setResizeMode( QHeaderView::Stretch );

    objectTree->header()->setStretchLastSection( true );
    objectTree->header()->setResizeMode( QHeaderView::Interactive );

    objectTree->setColumnCount( 3 );

    for ( int i = 0; i < 3; i++ ) {

        objectTree->setColumnWidth( i, 250 );

        //objectTree->setColumnWidth( i, applicationSettings->value( QString( "objecttree/column" + QString::number( i ) ), 350 ).toInt() );
    }

    QStringList labels;
    labels << " Type " << " Name " << " Id ";
    objectTree->setHeaderLabels ( labels );

}

// create properties dock widget
void MainWindow::createPropertiesDockWidget() {

    propertiesDock = new QDockWidget(tr(" Properties "), this);
    propertiesDock->setObjectName("properties");

    propertiesDock->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable );

    //propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    propertiesDock->setAllowedAreas(Qt::RightDockWidgetArea);

    tabWidget = new QTabWidget(gridLayoutWidget);
    tabWidget->setObjectName("properties");

    createPropertiesDockWidgetPropertiesTabWidget();
    createPropertiesDockWidgetMethodsTabWidget();
    createPropertiesDockWidgetSignalsTabWidget();
    createPropertiesDockWidgetApiTabWidget();

    tabWidget->setTabText( tabWidget->indexOf( propertiesTab ), QApplication::translate( "MainWindow", "A&ttributes", 0, QApplication::UnicodeUTF8 ) );
    tabWidget->setTabText( tabWidget->indexOf( methodsTab ), QApplication::translate( "MainWindow", "&Methods", 0, QApplication::UnicodeUTF8 ) );
    tabWidget->setTabText( tabWidget->indexOf( signalsTab ), QApplication::translate( "MainWindow", "&Signals", 0, QApplication::UnicodeUTF8 ) );
    tabWidget->setTabText( tabWidget->indexOf( apiTab ), QApplication::translate( "MainWindow", "AP&I", 0, QApplication::UnicodeUTF8 ) );

    tabWidget->setCurrentIndex( 0 );

    propertiesDock->setWidget(tabWidget);

    // add tabs to the dock
    addDockWidget(Qt::RightDockWidgetArea, propertiesDock, Qt::Horizontal);

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

void MainWindow::createPropertiesDockWidgetApiTabWidget() {

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
}


void MainWindow::createImageViewDockWidget()
{

    // Add imagewidged and resize checkbox
    imageViewDock = new QDockWidget( tr(" Image View "), this );
    imageViewDock->setObjectName("imageview");

    imageViewDock->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable );
    imageViewDock->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );

    QGroupBox * imageViewBox = new QGroupBox();
    imageViewBox->setObjectName("imageview");

    QVBoxLayout * layout = new QVBoxLayout;
    layout->setObjectName("imageview");

    // Image view
    imageWidget = new TDriverImageView( this, this );
    imageWidget->setObjectName("imageview");

    imageWidget->show();

    //    imageScrollArea = new QScrollArea;
    //  imageScrollArea->setObjectName("imageview");
    //    imageScrollArea->setBackgroundRole( QPalette::Dark );
    //    imageScrollArea->setWidget( imageWidget );
    //    imageScrollArea->setWidgetResizable( true );
    //    imageScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    //    imageScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    //    layout->addWidget( imageScrollArea );

    layout->addWidget( imageWidget );

    // image
    {
        QHBoxLayout *subLayout = new QHBoxLayout();

        checkBoxResize = new QCheckBox( tr( "Resi&ze image " ), this );
        checkBoxResize->setObjectName("imageview resize");
        checkBoxResize->show();
        subLayout->addWidget(checkBoxResize, 0, Qt::AlignLeft);

        subLayout->addWidget(new QLabel(tr("Left Click Action:")), 0, Qt::AlignRight);

        imageLeftClickChooser = new QComboBox();
        imageLeftClickChooser->addItem(tr("Inspect (no hover)"), TDriverImageView::VISUALIZER_INSPECT);
        imageLeftClickChooser->addItem(tr("Send tap to SUT"), TDriverImageView::SUT_DEFAULT_TAP);
        imageLeftClickChooser->setCurrentIndex(1);
        imageLeftClickChooser->addItem(tr("Insert coordinates to Code Editor"), TDriverImageView::ED_COORD_INSERT);
        imageLeftClickChooser->addItem(tr("Insert tap to Code Editor"), TDriverImageView::ED_TESTOBJ_INSERT);

        subLayout->addWidget(imageLeftClickChooser, 0, Qt::AlignLeft);

        layout->addLayout(subLayout);
    }


    connect( checkBoxResize, SIGNAL( stateChanged( int ) ), this, SLOT( changeImageResize() ) );
    connect( imageLeftClickChooser, SIGNAL(activated(int)), this, SLOT( changeImageLeftClick(int) ) );

    imageViewBox->setLayout( layout );
    imageViewDock->setWidget( imageViewBox );

    addDockWidget( Qt::LeftDockWidgetArea, imageViewDock, Qt::Horizontal );

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
            qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "Bad context menu result";

    }
    return result;
}




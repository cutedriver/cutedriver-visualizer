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


#ifndef TDRIVERMAINWINDOW_H
#define TDRIVERMAINWINDOW_H

#include <QtCore/QSettings>

#include <QPoint>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QClipboard>
#include <QtGui/QComboBox>
#include <QtGui/QDesktopServices>
#include <QtGui/QDockWidget>
#include <QtGui/QFileDialog>
#include <QtGui/QFont>
#include <QtGui/QFontDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLineEdit>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QPaintDevice>
#include <QtGui/QPainter>
#include <QtGui/QProgressBar>
#include <QtGui/QProgressDialog>
#include <QtGui/QPushButton>
#include <QtGui/QStackedLayout>
#include <QtGui/QStatusBar>
#include <QtGui/QTableWidget>
#include <QtGui/QTabWidget>
#include <QtGui/QTextEdit>
#include <QtGui/QTreeWidget>
#include <QtGui/QTreeWidgetItem>
#include <QtGui/QWidget>

#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtXml/QDomNode>
#include <QtXml/QXmlStreamReader>

#include "tdriver_behaviour.h"
#include <tdriver_util.h>

// visualizer UI classes
class TDriverRecorder;
class TDriverImageView;

// libeditor classes
class TDriverTabbedEditor;
class TDriverRunConsole;
class TDriverDebugConsole;
class TDriverRubyInteract;
class TDriverComboLineEdit;

class MainWindow : public QMainWindow {

    Q_OBJECT

public:

    QString tdriverPath;

    void checkInit();

    bool setup();
    bool checkVersion( QString current, QString required );
    bool collectMatchingVisibleObjects( QPoint pos, QList<int> &matchingObjects );

    // note: cancelAction must be first (lowest value) below
    enum ContextMenuSelection {
        cancelAction,
        copyAction,
        appendAction,
        insertAction,
        copyPathAction,
        appendPathAction,
        insertPathAction
    };

    bool isPathAction(ContextMenuSelection action) { return (action == copyPathAction || action == appendPathAction || action == insertPathAction); }

    enum ExecuteCommandType {
        commandSetOutputPath,
        commandListApps,
        commandClassMethods,
        commandDisconnectSUT,
        commandRecordingStart,
        commandRecordingStop,
        commandRecordingTest,
        commandTapScreen,
        commandRefreshUI,
        commandKeyPress,
        commandSetAttribute,
        commandGetDeviceType,
        commandCheckApiFixture,
        commandBehavioursXml,
        commandGetVersionNumber,
        commandSignalList
    };

    enum commandResult {

        WARNING = 0x1,
        FAIL = 0x2,
        RETRY = 0x4,
        DISCONNECT = 0x08,
        SILENT = 0x10
             };

public:    // methods to access test object data by object id
    const QMap<QString, QHash<QString, QString> > &testobjAttributes(int id) { return attributesMap[id]; }
    const QStringList &testobjGeometries(int id) { return geometriesMap[id]; }
    const QTreeWidgetItem *testobjTreeWidget(int id) { return objectTreeMap[id]; }
    const QHash<QString, QString> &testobjTreeData(int id) { return objectTreeData[id]; }

private:

    // command executing & error handling

    bool processErrorMessage( QString & resultMessage, ExecuteCommandType commandType, int & resultEnum );
    bool execute_command( ExecuteCommandType commandType, QString commandString, QString additionalInformation = QString(), BAListMap *reply = NULL);

    // global data & caches

    bool offlineMode;

    TDriverRecorder *mRecorder;
    //Assistant *tdriverAssistant;

    QSettings *applicationSettings;
    QString outputPath;

    QFont *defaultFont;
    QString parametersFile;

    TDriverImageView *imageWidget;

    QMap<QString, QHash<QString, QString> > deviceList;
    QMap<QString, QString> activeDevice;
    //        QString activeApplication;

    QHash<QString, QString> applicationsHash;
    QMap<int, QString> applicationsProcessIdMap;

    QMap<int, QMap<QString, QHash<QString, QString> > > attributesMap;
    QHash<QString, QMap<QString, QHash<QString, QString> > > apiMethodsMap;
    QHash<QString, QStringList > apiSignalsMap;
    QMap<QString, Behaviour> behavioursMap;

    QMap<int, QStringList> geometriesMap;
    QList<int> visibleObjectsList;

    QMap<int, QTreeWidgetItem *> objectTreeMap;
    QMap<int, QHash<QString, QString> > objectTreeData;
    QHash<QString, QMap<QString, QString> > objectMethods;
    QHash<QString, QMap<QString, QString> > objectSignals;

    //QString strActiveAppName;
    //QString strCurrentApplication;

    // ui

    QGroupBox *horizontalBottomButtonGroupBox;
    QGroupBox *verticalNavigationButtonGroupBox;

    QWidget *gridLayoutWidget;
    QVBoxLayout *gridLayout;


    // helper functions
    static QByteArray cleanDoneResult(QByteArray output);


    void updateWindowTitle();

    void setActiveDevice( QString deviceName );
    QString getDeviceType( QString deviceName );
    QString getDriverVersionNumber();

    void noDeviceSelectedPopup();

    ContextMenuSelection showCopyAppendContextMenu();

    bool isDeviceSelected();

    QString selectFolder( QString title, QString filter, QFileDialog::AcceptMode mode );

    bool createStateArchive( QString target );

    void statusbar( QString text, int currentProgressValue, int maxProgressValue, int timeout = 0 );
    void statusbar( QString text, int timeout = 0 );

    // properties widget

    QTabWidget *tabWidget;

    // properties widget
    void createPropertiesDockWidget();
    void createPropertiesDockWidgetPropertiesTabWidget();
    void createPropertiesDockWidgetMethodsTabWidget();
    void createPropertiesDockWidgetSignalsTabWidget();
    void createPropertiesDockWidgetApiTabWidget();


    QTableWidget *propertiesTable;
    QTableWidget *methodsTable;
    QTableWidget *signalsTable;
    QTableWidget *apiTable;

    QStackedLayout *propertiesLayout;
    QStackedLayout *methodsLayout;
    QStackedLayout *signalsLayout;
    QStackedLayout *apiLayout;

    QWidget *propertiesTab;
    QWidget *methodsTab;
    QWidget *signalsTab;
    QWidget *apiTab;

    void updatePropetriesTable();
    void clearPropertiesTableContents();

    void updateAttributesTableContent();
    void updateMethodsTableContent();
    void updateSignalsTableContent();
    void updateApiTableContent();

    void getClassMethods( QString objectType );
    void getClassSignals( QString objectType, QString objectId );

    // object tree

    QTreeWidget *objectTree;

    void createTreeViewDockWidget();

    //QTreeWidgetItem *currentObjectTreeItem;
    int collapsedObjectTreeItemPtr;
    int expandedObjectTreeItemPtr;

    void resizeObjectTree();

    QMap<QString, int> propertyTabLastTimeUpdated;

    QMap<QString, QString> currentApplication;

    bool foregroundApplication;

    //QString applicationIdFromXml;

    void clearObjectTreeMappings();
    void updateObjectTree( QString filename );

    bool parseObjectTreeXml( QString filename, QDomDocument &resultDomTree );
    void buildObjectTree( QTreeWidgetItem *parentItem, QDomElement parentElement );
    void storeItemToObjectTreeMap( QTreeWidgetItem *item, QString type, QString name, QString id );

    QTreeWidgetItem * createObjectTreeItem( QTreeWidgetItem *parentItem, QString type, QString name, QString id );

    void objectTreeItemChanged();

    void collectGeometries( QTreeWidgetItem *item );
    void collectGeometries( QTreeWidgetItem *item, QStringList &geometries );

    bool splitGeometry( QString geometry, QStringList &geometryList );
    bool geometryContains( QString geometry, int x, int y );
    QRect convertGeometryToQRect( QString geometry );
    int geometrySize( QString geometry );

    bool getParentItemOffset( QTreeWidgetItem *item, float &x, float &y );

    void objectTreeKeyPressEvent( QKeyEvent * event );

    // libeditor ui
    QDockWidget *editorDock;
    QDockWidget *runDock;
    QDockWidget *debugDock;
    QDockWidget *irDock;
    void createEditorDocks();
    TDriverTabbedEditor *tabEditor;
    TDriverRunConsole *runConsole;
    TDriverDebugConsole *debugConsole;
    TDriverRubyInteract *irConsole;

    // xml
    bool parseXml( QString fileName, QDomDocument &resultDocument );

    // ui dump xml
    QDomDocument xmlDocument;

    // behaviours xml
    QDomDocument behaviorDomDocument;

    // tdriver_parameters.xml
    bool getXmlParameters( QString filename );
    void updateDevicesList(const QMap<QString, QHash<QString, QString> > &newDeviceList);

    // visualizer_applications_sut_id.xml
    void parseApplicationsXml( QString filename );
    void updateApplicationsList();
    void resetApplicationsList();

    // behaviours.xml
    void parseBehavioursXml( QString filename );
    void buildBehavioursMap();

    bool updateBehaviourXml();

    // visualizer_dump_sut_id.xml
    void parseUiDump( QString filename );

    // api fixture
    bool apiFixtureEnabled;
    bool apiFixtureChecked;
    bool checkApiFixture();
    void parseApiMethodsXml( QString filename );
    QStringList parseSignalsXml( QString filename );

    // other methods

    void connectSignals();
    void connectObjectTreeSignals();
    void connectTabWidgetSignals();
    void connectImageWidgetSignals();

    // setup

    void createUi();
    void createGridLayout();
    void createTopMenuBar();
    void createDock();

    void setupTableWidgetHeader( QString headers, QTableWidget *table);

    // menu

    void createFileMenu();
    void createViewMenu();
    void createSearchMenu();
    void createDevicesMenu();
    void createApplicationsMenu();
    void createRecordMenu();
    void createHelpMenu();

    QMenuBar *menubar;

    QMenu *fileMenu;
    QMenu *viewMenu;
    QMenu *viewMenuDockWidgets;
    QMenu *viewMenuObjectTree;
    QMenu *searchMenu;
    QMenu *deviceMenu;
    QMenu *appsMenu;
    QMenu *recordMenu;
    QMenu *helpMenu;

    // help
    QAction *visualizerAssistant;
    QAction *visualizerHelp;
    QAction *aboutVisualizer;

    // file
    QAction *parseSUT;
    QAction *loadXmlAction;
    QAction *saveStateAction;
    QAction *fontAction;
    QAction *refreshAction;
    QAction *disconnectCurrentSUT;
    QAction *exitAction;

    // view
    QAction *viewButtons;
    QAction *viewImage;
    QAction *viewProperties;
    QAction *viewShortcuts;
    QAction *viewEditor;
    QAction *viewClipboard;
    QAction *viewExpandAll;
    QAction *viewCollapseAll;
    QAction *showXmlAction;

    // search
    QAction *findAction;

    // record
    QAction *recordAction;


    // image

    QCheckBox *checkBoxResize;
    QComboBox *imageLeftClickChooser;
    //QString lastScreenShotApplication;

    void tapScreen( QString target );

    void createImageViewDockWidget();

    // highlight

    int lastHighlightedObjectPtr;
    void drawHighlight( int itemPtr );


    bool highlightById( int id, bool selectItem, QString insertMethodToEditor = QString() );
    bool highlightAtCoords( QPoint pos, bool selectItem, QString insertMethodToEditor = QString() );
    // insertMethodToEditor.isNull means don't insert,
    // insertMethodToEditor.isEmpty means insert without method name

    bool getSmallestObjectFromMatches( QList<int> *matchingObjects, int &objectPtr );

    // shortcuts widget

    void createShortcuts();

    QPushButton *refreshButton;
    QPushButton *delayedRefreshButton;
    QPushButton *loadFileButton;
    QPushButton *showXMLButton;
    QPushButton *quitButton;

    // s60 keyboard widget

    void createKeyboardCommands();

    // docks

    // clipboard
    QDockWidget *clipboardDock;
    QLineEdit *objectAttributeLineEdit;

    // image
    QDockWidget *imageViewDock;

    // shortcuts
    QDockWidget *shortcutsDock;

    // buttons / keyboard commands
    QDockWidget *keyboardCommandsDock;

    // properties
    QDockWidget *propertiesDock;

    // show xml

    void createXMLFileDataWindow();

    // store find dialog position
    QPoint xmlViewPos;

    QDialog *xmlView;
    QTextEdit *sourceEdit;
    QComboBox *findStringComboBox;

    QCheckBox *showXmlMatchCase;
    QCheckBox *showXmlMatchEntireWord;
    QCheckBox *showXmlBackwards;
    QCheckBox *showXmlWrapAround;

    QPushButton *showXmlFindButton;
    QPushButton *showXmlCloseButton;

    // find dialog

    void createFindDialog();
    void createFindDialogShortcuts();
    QPoint findDialogPos;

    QCheckBox *findDialogMatchCase;
    QCheckBox *findDialogBackwards;
    QCheckBox *findDialogEntireWords;
    QCheckBox *findDialogWrapAround;
    QCheckBox *findDialogAttributes;
    QCheckBox *findDialogSubtreeOnly;

    TDriverComboLineEdit *findDialogText;

    QDialog *findDialog;
    QPushButton *findDialogFindButton;
    QPushButton *findDialogCloseButton;
    QTreeWidgetItem *findDialogSubtreeRoot;

    bool containsWords( QHash<QString, QString> itemData, QString text, bool caseSensitive, bool entireWords  );
    bool attributeContainsWords( int itemPtr, QString text, bool caseSensitive, bool entireWords );

signals:
    void defaultFontSet(QFont font);
    void insertToCodeEditor(QString text, bool prependParent, bool prependDot);
    void disconnectSUTResult(bool disconnected);

private slots:

    // global

    void collapseTreeWidgetItem( QTreeWidgetItem *item );
    void expandTreeWidgetItem( QTreeWidgetItem *item );

    void tabWidgetChanged( int currentTableWidget );

    void methodItemPressed( QTableWidgetItem *item );
    void propertiesItemPressed( QTableWidgetItem *item );
    void apiItemPressed( QTableWidgetItem *item );

    void keyPressEvent ( QKeyEvent *event );
    bool eventFilter ( QObject *obj, QEvent *event );

    void showMainVisualizerAssistant();
    void showContextVisualizerAssistant( const QString &page );

    void showVisualizerHelp();
    void showAboutVisualizer();

    void resizeEvent( QResizeEvent *event );
    void closeEvent( QCloseEvent *event );

    // clipboard

    void createClipboardDock();

    void updateClipboardText( QString text, bool appendText );
    void changePropertiesTableValue( QTableWidgetItem *item );


    // object tree
    void delayedRefreshData();
    void forceRefreshData();

    void refreshData();
    void refreshDataDisplay();

    void objectViewItemClicked( QTreeWidgetItem *item, int column );
    void objectViewItemAction( QTreeWidgetItem *item, int column, ContextMenuSelection action, QString method = QString() );
    void objectViewCurrentItemChanged( QTreeWidgetItem *itemCurrent, QTreeWidgetItem *itemPrevious );

    // menu: file

    void getParameterXML();

    void loadFileData();
    void saveStateAsArchive();
    void clickedImage();

    void openFontDialog();
    bool disconnectSUT();
    bool disconnectExclusiveSUT();

    // menu: view

    // object tree: expand all
    void objectTreeExpandAll();

    // object tree: collapse all
    void objectTreeCollapseAll();

    // show xml
    void showXMLDialog();

    // image

    void imageInspectFindItem();
    void imageInsertFindItem();
    void imageInsertCoords();

    void imageInsertObjectFromId(int id);
    void imageInspectFromId(int id);
    void imageTapFromId(int id);


    void changeImageResize();
    void changeImageLeftClick(int index);

    // menu: applications

    void appSelected();

    // menu: devices

    void deviceSelected();


    // menu: view

    void changeKeyboardCommandsVisibility();
    void changeShortcutsVisibility();
    void changeEditorVisibility();
    void changePropertiesVisibility();
    void changeImagesVisibility();
    void changeClipboardVisibility();

    void visiblityChangedClipboard( bool state );
    void visiblityChangedImage( bool state );
    void visiblityChangedProperties( bool state );
    void visiblityChangedShortcuts( bool state );
    void visiblityChangedEditor( bool state );
    void visiblityChangedButtons( bool state );

    // menu: record

    void openRecordWindow();

    // dock: keyboard commands

    void deviceActionButtonPressed();

    // show xml

    void findStringFromXml();
    void closeXmlDialog();

    void showXmlEditTextChanged( const QString &text );

    // find dialog

    void showFindDialog();
    void findNextTreeObject();

    void findDialogTextChanged( const QString & text );
    void findDialogHandleTreeCurrentChange(QTreeWidgetItem*current);
    void findDialogSubtreeChanged( int value);
    void closeFindDialog();

private:
    QString treeObjectIdentification(int treeItemPtr, int sutItemPtr);
    QTreeWidgetItem *findDialogSubtreeNext(QTreeWidgetItem *current, QTreeWidgetItem *root, bool wrap=false);
    QTreeWidgetItem *findDialogSubtreePrev(QTreeWidgetItem *current, QTreeWidgetItem *root, bool wrap=false);
    bool compareTreeItem(QTreeWidgetItem *item, const QString &findString, bool matchCase, bool entireWords, bool searchAttributes);
    void findFromSubTree(QTreeWidgetItem *current, const QString &findString, bool backwards, bool matchCase, bool entireWords, bool searchWrapAround, bool searchAttributes);

};

#endif

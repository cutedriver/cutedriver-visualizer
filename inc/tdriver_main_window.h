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
#include <QtGui/QTreeWidget>
#include <QtGui/QTreeWidgetItem>
#include <QtGui/QWidget>

#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtXml/QDomNode>
#include <QtXml/QXmlStreamReader>

class QErrorMessage;
class QScrollArea;
class QToolBar;
class QPlainTextEdit;

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

#include "tdriver_main_types.h"

#define DOCK_FEATURES_DEFAULT (QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable)

#define DEVICE_BUTTONS_ENABLED 0

class MainWindow : public QMainWindow {

    Q_OBJECT

public:
    MainWindow();

    QString tdriverPath;

    void checkInit();

    bool setup();
    bool checkVersion( QString current, QString required );
    bool collectMatchingVisibleObjects( QPoint pos, QList<TestObjectKey> &matchingObjects );

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
        commandRefreshImage,
        commandKeyPress,
        commandSetAttribute,
        commandGetDeviceType,
        commandCheckApiFixture,
        commandBehavioursXml,
        commandGetVersionNumber,
        commandSignalList,
        commandGetDeviceParameter
    };

    enum ExecuteCommandResult {
        OK = 0,
        WARNING = 0x1,
        FAIL = 0x2,
        RETRY = 0x4,
        DISCONNECT = 0x08,
        SILENT = 0x10
             };

public:    // methods to access test object data by object id
    const QMap<QString, QHash<QString, QString> > &testobjAttributes(TestObjectKey id) { return attributesMap[id]; }
    //const QStringList &testobjGeometries(AttributeKey id) { return geometriesMap[id]; }
    //const QTreeWidgetItem *testobjTreeWidget(AttributeKey id) { return objectTreeMap[id]; }
    const QHash<QString, QString> &testobjTreeData(TestObjectKey id) { return objectTreeData[id]; }

public slots:
    void refreshScreenshotObjectList();
    void statusbar( QString text, int currentProgressValue, int maxProgressValue, int timeout = 0 );
    void statusbar( QString text, int timeout = 0 );
    void handleRbiError(QString title, QString text, QString details);

private:

    // command executing & error handling

    void processErrorMessage( ExecuteCommandType commandType, const BAListMap &msg, const QString &additionalInformation,
                              unsigned &resultEnum, QString &clearError, QString &shortError, QString &fullError );
    bool executeTDriverCommand( ExecuteCommandType commandType, const QString &commandString, const QString &additionalInformation = QString(), BAListMap *reply = NULL);

    // global data & caches

    bool offlineMode;

    TDriverRecorder *mRecorder;

    QSettings *applicationSettings;
    QString outputPath;

    QFont *defaultFont;
    QString parametersFile;

    QScrollArea *imageScroller;
    TDriverImageView *imageWidget;

    QMap<QString, QHash<QString, QString> > deviceList;
    QMap<QString, QString> activeDevice;
    //        QString activeApplication;

    QMap<QString, QString> tdriverXmlParameters;

    QMap<QString, QString> applicationsNamesMap;
    QMap<QAction*, QString> applicationsActionMap;

    QMap<TestObjectKey, QMap<QString, QHash<QString, QString> > > attributesMap;
    QHash<QString, QMap<QString, QHash<QString, QString> > > apiMethodsMap;
    QHash<QString, QStringList > apiSignalsMap;
    QMap<QString, Behaviour> behavioursMap;

    QMap<TestObjectKey, RectList> geometriesMap;
    QSet<TestObjectKey> screenshotObjects;

    QMap<TestObjectKey, QHash<QString, QString> > objectTreeData;
    QHash<QString, TestObjectKey> objectIdMap;
    QHash<QString, QMap<QString, QString> > objectMethods;
    QHash<QString, QMap<QString, QString> > objectSignals;


    // helper functions
    static QByteArray cleanDoneResult(QByteArray output);

    void updateWindowTitle();

    void setActiveDevice( QString deviceName );
    QString getDeviceType( QString deviceName );
    QString getDeviceParameter( QString deviceName, QString parameter );
    QString getDriverVersionNumber();

    void noDeviceSelectedPopup();

    ContextMenuSelection showCopyAppendContextMenu();

    bool isDeviceSelected();

    QString selectFolder( QString title, QString filter, QFileDialog::AcceptMode mode );

    bool createStateArchive( QString target );

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

    void updatePropertiesTable();
    void clearPropertiesTableContents();

    void updateAttributesTableContent();
    void updateMethodsTableContent();
    void updateSignalsTableContent();
    void updateApiTableContent();

    void getClassMethods( QString objectType );
    bool getClassSignals( QString objectType, QString objectId );

    // object tree

    QTreeWidget *objectTree;
    QString uiDumpFileName;

    void createTreeViewDockWidget();

    //QTreeWidgetItem *currentObjectTreeItem;
    TestObjectKey collapsedObjectTreeItemPtr;
    TestObjectKey expandedObjectTreeItemPtr;

    void resizeObjectTree();

    QMap<QString, TestObjectKey> propertyTabLastTimeUpdated;

    ApplicationInfo currentApplication;

    bool foregroundApplication;

    //QString applicationIdFromXml;

    void clearObjectTreeMappings();
    void updateObjectTree( QString filename );

    bool parseObjectTreeXml( QString filename, QDomDocument &resultDomTree );
    void buildScreenshotObjectList(TestObjectKey parentKey=0);
    void buildObjectTree( QTreeWidgetItem *parentItem, QDomElement parentElement );
    void storeItemToObjectTreeMap( QTreeWidgetItem *item, QString type, QString name, QString id );

    QTreeWidgetItem * createObjectTreeItem( QTreeWidgetItem *parentItem, QString type, QString name, QString id );

    void objectTreeItemChanged();

    void collectGeometries( QTreeWidgetItem *item, RectList &geometries);

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

    void createActions();
    void createUi();
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
    QMenu *searchMenu;
    QMenu *deviceMenu;
    QMenu *appsMenu; // must be kept in sync with appsBar!
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
    QAction *delayedRefreshAction;
    QAction *disconnectCurrentSUT;
    QAction *exitAction;

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

    TestObjectKey lastHighlightedObjectKey;
    void drawHighlight( TestObjectKey itemKey, bool multiple );


    bool highlightByKey( TestObjectKey itemKey, bool selectItem, QString insertMethodToEditor = QString() );
    bool highlightAtCoords( QPoint pos, bool selectItem, QString insertMethodToEditor = QString() );
    // insertMethodToEditor.isNull means don't insert,
    // insertMethodToEditor.isEmpty means insert without method name

    bool getSmallestObjectFromMatches( QList<TestObjectKey> *matchingObjects, TestObjectKey &objectPtr );


#if DEVICE_BUTTONS_ENABLED
    // s60 keyboard widget
    void createKeyboardCommands();
#endif

    // toolbars

    QToolBar *appsBar; // must be kept in sync with appsMenu!
    QToolBar *shortcutsBar;

    // clipboard toolbar
    QToolBar *clipboardBar;
    QLineEdit *objectAttributeLineEdit;

    // image
    QDockWidget *imageViewDock;


#if DEVICE_BUTTONS_ENABLED
    // buttons / keyboard commands
    QDockWidget *keyboardCommandsDock;
#endif

    // properties
    QDockWidget *propertiesDock;

    // show xml

    void createXMLFileDataWindow();

    // store find dialog position
    QPoint xmlViewPos;

    QDialog *xmlView;
    QPlainTextEdit *sourceEdit;
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
    bool attributeContainsWords( TestObjectKey itemPtr, QString text, bool caseSensitive, bool entireWords );

    QErrorMessage *tdriverMsgBox;
    int tdriverMsgTotal;
    int tdriverMsgShown;

signals:
    void defaultFontSet(QFont font);
    void insertToCodeEditor(QString text, bool prependParent, bool prependDot);
    void disconnectSUTResult(bool disconnected);

private slots:

    // global
    void tdriverMsgSetTitleText();
    void tdriverMsgOkClicked();
    void tdriverMsgFinished();
    void tdriverMsgAppend(QString message);

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

    void closeEvent( QCloseEvent *event );

    // toolBar methods

    void createAppsBar();
    void createShortcutsBar();
    void createClipboardBar();

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

    void imageInsertObjectFromId(TestObjectKey id);
    void imageInspectFromId(TestObjectKey id);
    void imageTapFromId(TestObjectKey id);


    void changeImageResize(bool resize);
    void changeImageLeftClick(int index);

    // menu: applications

    void appSelected();

    // menu: devices

    void deviceSelected();

    // menu: view

    // menu: record

    void openRecordWindow();

#if DEVICE_BUTTONS_ENABLED
    // dock: keyboard commands
    void deviceActionButtonPressed();
#endif

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
    QString treeObjectRubyId(TestObjectKey treeItemPtr, TestObjectKey sutItemPtr);
    QTreeWidgetItem *findDialogSubtreeNext(QTreeWidgetItem *current, QTreeWidgetItem *root, bool wrap=false);
    QTreeWidgetItem *findDialogSubtreePrev(QTreeWidgetItem *current, QTreeWidgetItem *root, bool wrap=false);
    bool compareTreeItem(QTreeWidgetItem *item, const QString &findString, bool matchCase, bool entireWords, bool searchAttributes);
    void findFromSubTree(QTreeWidgetItem *current, const QString &findString, bool backwards, bool matchCase, bool entireWords, bool searchWrapAround, bool searchAttributes);

};

#endif

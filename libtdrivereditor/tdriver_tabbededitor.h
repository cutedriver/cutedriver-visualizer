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


#ifndef TDRIVER_TABBEDEDITOR_H
#define TDRIVER_TABBEDEDITOR_H

#include "tdriver_editor_common.h"
#include "tdriver_rubyhighlighter.h"

#include <QTabWidget>
#include <QFont>

class QAction;
class QDockWidget;
class QShowEvent;
class QHideEvent;
class QMenuBar;
class QSettings;
class QCloseEvent;

class TDriverEditBar;
class TDriverRunConsole;
class TDriverDebugConsole;
class TDriverRubyInteract;
class TDriverCodeTextEdit;

#include <QList>
#include <QTextCursor>

#include "tdriver_runconsole.h"

class TDriverTabbedEditor : public QTabWidget
{
    Q_OBJECT
    Q_PROPERTY(QString sutVariable READ sutVariable)
    Q_PROPERTY(QString appVariable READ appVariable)

public:
    explicit TDriverTabbedEditor(QWidget *shortcutParent = 0, QWidget *parent = 0);

    void setParamMap(const QMap<QString, QString> &map);
    TDriverEditBar *searchBar() { return editBarP; }

    const QList<QAction *> &fileActions() const { return fileActs; }
    const QList<QAction *> &recentFileActions() const { return recentFileActs; }
    const QList<QAction *> &editActions() const { return editActs; }
    const QList<QAction *> &codeActions() const { return codeActs; }
    const QList<QAction *> &optionActions() const { return optActs; }
    const QList<QAction *> &runActions() const { return runActs; }

    QString sutVariable() const;
    QString appVariable() const;

    QMenuBar *createEditorMenuBar(QWidget *parent = 0);

    bool currentHasWritableCursor();

    enum { MaxRecentFiles=6 };
    enum ActionContext { BeforeExit, BeforeRun };

protected:
    virtual void showEvent(QShowEvent *);
    virtual void hideEvent(QHideEvent *);
    virtual void dragEnterEvent(QDragEnterEvent *);
    virtual void dropEvent(QDropEvent *);

signals:
    void requestRun(QString filename, TDriverRunConsole::RunRequestType);
    void requestRunPreparations(QString filename);

    // breakpoint signals below are just forwarders for TDriverCodeTextEdit signals
    void addedBreakpoint(struct MEC::Breakpoint);
    void removedBreakpoint(int rdebugInd);
    void dataSyncDone();

    void documentNameChanged(QString filename);

public slots:
    void setEditorFont(QFont font);

    void runFilePrep(QString fileName, TDriverRunConsole::RunRequestType type);
    bool proceedRun();

    void addBreakpoint(struct MEC::Breakpoint);
    void addBreakpointList(QList<struct MEC::Breakpoint>);
    void setRunningLine(const QString file, int lineNum);
    void resetRunningLines();
    void resetBreakpoints();
    void syncTabsData();

    bool queryUnsavedFate(ActionContext);

    bool smartInsert(QString text, bool prependParent, bool prependDot);

    void currentChangeAction(int index);
    void focusCurrent();
    bool open(void);
    bool saveCurrent(void);
    bool saveCurrentAs(void);
    bool saveCurrentAsTemplate(void);
    bool saveAll(void);
    bool closeCurrent(void);
    void nextTab();
    void prevTab();
    bool openRecentFile();

    void newFile(QString fileName = QString());
    bool newFromTemplate(void);

    bool run(void);
#if TDRIVER_RUNCONSOLE_DEBUG1_ENABLED
    bool debug1(void);
#endif
    bool debug2(void);

    bool saveTab(int index);
    bool saveTabAs(int index, const QString &caption, const QString &filter=QString());
    bool closeTab(int index);

    void setRunConsoleVisible(bool);
    void setDebugConsoleVisible(bool);
    void setIRConsoleVisible(bool);
    void showIrConsole();

    bool mainCloseEvent(QCloseEvent *);

    void connectConsoles(TDriverRunConsole *runConsole, QWidget *runContainer,
                         TDriverDebugConsole *debugConsole, QWidget *debugContainer,
                         TDriverRubyInteract *iConsole, QWidget *iContainer);

    void editorModeChange();
    void documentModification(bool);

    // these (and newFile() above) allow programmatically setting up editor files
    bool loadFile(QString fileName, bool fromTemplate = false);
    bool saveFile(QString fileName, int index, bool resetEncoding);
    void recentFileUpdate(QString fileName);
    void updateRecentFileActions();

private slots:
    TDriverCodeTextEdit *prepareExtAction();

    void updateTab(int index=-1);


private:
    QMap<QString, QString> paramMap;
    QFont editorFont;
    QString proceedRunFilename;
    TDriverRunConsole::RunRequestType proceedRunType;
    bool proceedRunPending;

    TDriverEditBar *editBarP;

    TDriverRubyHighlighter *rubyHighlighter;
    TDriverHighlighter *plainHighlighter;
    void createActions();

    QWidget *runConsoleContainer;
    TDriverRunConsole *runConsole;
    bool runConsoleVisible;
    QWidget *debugConsoleContainer;
    TDriverDebugConsole *debugConsole;
    bool debugConsoleVisible;

    QWidget *irConsoleContainer;
    TDriverRubyInteract *irConsole;
    bool irConsoleVisible;

    // these functions handle dynamic connection of signals that go directly into thte open tap
    void disconnectTabSignals();
    void connectTabSignals(TDriverCodeTextEdit *);

    QList<QAction *> fileActs;
    QList<QAction *> editActs;
    QList<QAction *> codeActs;
    QList<QAction *> optActs;
    QList<QAction *> runActs;
    QList<QAction *> recentFileActs;

    // file actions
    QAction *newAct;
    QAction *newFromTemplateAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *saveAsTemplateAct;
    QAction *saveAllAct;
    QAction *closeAct;

    // edit actions
    QAction *undoAct;
    QAction *redoAct;
    QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;
    QAction *selectAllAct;


    // code manipulation actions
    QAction *commentCodeAct;

    // option actions
    QAction *toggleUsingTabulatorsModeAct;
    QAction *toggleRubyModeAct;
    QAction *toggleWrapModeAct;

    QAction *toggleRunDockAct;
    QAction *toggleDebugDockAct;
    QAction *toggleIrDockAct;

    // run actions
    QAction *runAct;
#if TDRIVER_RUNCONSOLE_DEBUG1_ENABLED
    QAction *debug1Act;
#endif
    QAction *debug2Act;
};

#endif // TDRIVER_TABBEDEDITOR_H

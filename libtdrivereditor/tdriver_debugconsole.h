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


#ifndef TDRIVER_DEBUGCONSOLE_H
#define TDRIVER_DEBUGCONSOLE_H

#include "tdriver_editor_common.h"
#include <QWidget>

class QVBoxLayout;
class TDriverComboLineEdit;
class QComboBox;
class QPushButton;
class QAction;
class QTcpSocket;
class QToolBar;
class TDriverConsoleTextEdit;
class TDriverRunConsole;

#include <QStringList>

class TDriverDebugConsole : public QWidget
{
    Q_OBJECT
public:
    explicit TDriverDebugConsole(QWidget *parent = 0);
    QTcpSocket &connection() { return *remoteConn; }


signals:
    void connectResult(bool);
    void requestDataSync();

    void runStarting(QString fileName, int lineNum); // emitted when rdebug continues script execution, starting from line
    void gotRemotePrompt(QString fileName, int lineNum); // emitted when rdebug gives a position in a file
    void breakpoint(struct MEC::Breakpoint);
    void breakpoints(QList<struct MEC::Breakpoint>);
    void delegateContinueAction();

public slots:
    void clear();
    void resetConnections();
    void connectTo(QString host, quint16 dport, quint16 cport, TDriverRunConsole *procConsoleOwner);
    void resetProcConsoleOwner();
    void setRunning(bool);

    void handleRemoteText(QString text);
    void handleControlText(QString text);
    void readRemoteText();
    void readControlText();

    void updateActionStates();

    void doQuit(void);
    void doInterrupt(void);
    void doContinueOrDelegate(void);
    void doStepInto(void);
    void doStepOver(void);

    void allSynced();
    void addBreakpoint(struct MEC::Breakpoint);
    void removeBreakpoint(int rdebugInd);
    void removeAllBreakpoints();

private:
    void createActions();

    // debugConn is actual debugger interface, which will be unresponsive when script is running
    QTcpSocket *remoteConn;
    // controlConn is hidden from user, and most importantly used to give command "interrupt"
    QTcpSocket *controlConn;

    QToolBar *toolbar;
    QVBoxLayout *layout;
    TDriverConsoleTextEdit *remoteConsole;
    TDriverConsoleTextEdit *controlConsole;
    TDriverComboLineEdit *comboLine;
    QComboBox *cmdTargetCB;
    QPushButton *sendCmdButton;

    bool remoteHasPrompt;
    bool controlHasPrompt;

    TDriverRunConsole *procConsoleOwner;
    bool isRunning;
    bool syncingBeforeContinue;
    bool dataSynced;

    bool quitWaiting;
    bool quitSent;
    bool interruptWaiting;
    bool interruptSent;

    QString rdebugDelim;
    QString remoteParseKey;

    QMap<QString, QStringList> *remoteParsedLast;
    QMap<QString, QStringList> *remoteParsedNew;

    QString remotePromptPrefix;
    QString remoteBuffer;

    QString controlPromptPrefix;
    QString controlBuffer;

    QStringList remoteCmdQueue;
    QStringList controlCmdQueue;

    QAction *clearAct;
    QAction *quitAct;
    QAction *interruptAct;
    QAction *continueAct;
    QAction *stepOverAct;
    QAction *stepIntoAct;

private slots:
    void cmdToDebugger();
    // below QString() produces null QString, meaning use the queue
    // empty but non-null cmd will send just newline, so null is needed
    bool sendRemoteCmd(QString cmd=QString(), bool allowQueuing=false);
    bool sendControlCmd(QString cmd=QString(), bool allowQueuing=false);
    void emitRunningPosition(QMap<QString, QStringList> &remoteParsed, bool starting);
};

#endif // TDRIVER_DEBUGCONSOLE_H

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


#ifndef TDRIVER_RUNCONSOLE_H
#define TDRIVER_RUNCONSOLE_H


#include <QWidget>
#include <QProcess>

class QVBoxLayout;
class QAction;
class TDriverConsoleTextEdit;
class QToolBar;
class QLabel;

#define TDRIVER_RUNCONSOLE_DEBUG1_ENABLED 0

class TDriverRunConsole : public QWidget
{
Q_OBJECT
public:

    enum RunRequestType { RunRequest=0, Debug1Request=1, Debug2Request=2, InteractRequest=3 };

    explicit TDriverRunConsole(bool makeProc=true, QWidget *parent = 0);
    ~TDriverRunConsole();

    QProcess &process() { return *proc; }
    bool isRunning() { return (proc && (proc->state() == QProcess::Running)); }

signals:
    void requestRemoteDebug(QString host, quint16 dport, quint16 cport, TDriverRunConsole *procConsoleOwner);
    void lineForRdebugConsole(QString);
    void needDebugConsole(bool);

    //void processStateChanged(QProcess::ProcessState); // redirected proc->stateChanged signal
    void runSignal(bool); // emitted as true when trying to run a proc, as false when proc ends or run fails
    void runningState(bool); // emitted as true when process is actually running, as false when process ends

public slots:
    virtual void clear();
    virtual bool runFile(QString fileName, TDriverRunConsole::RunRequestType);
    virtual void rdebugStarted(void);
    virtual void rdebugReady(void);
    virtual void setStdStreamsHidden(bool state);
    //virtual void setStdStreamsVisible(bool state);
    virtual void endProcess(void);

protected slots:
    virtual void procError(QProcess::ProcessError);
    virtual void procFinished(int, QProcess::ExitStatus);
    virtual void procStarted(void);
    virtual void readProcText();

protected:
    void createActions();

    QProcess *proc;

    QToolBar *toolbar;
    QVBoxLayout *layout;
    QLabel *commandLineLabel;
    TDriverConsoleTextEdit *console;

    enum { NO_OUTPUT, PROCESS_OUTPUT, RDEBUG_OUTPUT } outputMode;

    QAction *clearAct;
    QAction *terminateAct;
    QAction *killAct;

    const QString RubyProg;
    const QString RdebugProg;

};



#endif // TDRIVER_RUNCONSOLE_H

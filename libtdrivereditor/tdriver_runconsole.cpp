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


#include "tdriver_runconsole.h"
#include "tdriver_consoletextedit.h"
#include "tdriver_combolineedit.h"
#include "tdriver_editor_common.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAction>
#include <QToolBar>
#include <QProcess>
#include <QTcpSocket>
#include <QTimer>
#include <QMessageBox>
#include <QFileInfo>
#include <QCoreApplication>
#include <QLabel>
#include <QDir>
#include <QSettings>

#include <tdriver_util.h>
#include <tdriver_debug_macros.h>


static quint16 remotePort = 47747; // note: ruby-debug 0.10.0 with ruby 1.8 default would be 8989
static quint16 controlPort = 47748; // note: ruby-debug 0.10.0 with ruby 1.8 default would be 8990

static const char TDRIVER_RUBY_INTERACTSCRIPT[] = "tdriver_rubyinteract.rb";

#ifdef Q_OS_WIN
#define DEFAULT_RUBY_BIN "ruby.exe"
#define DEFAULT_RDEBUG_BIN "ruby.exe"
#define DEFAULT_RDEBUG_ARGS (QStringList() << "C:/Ruby/bin/rdebug")
#else
#define DEFAULT_RUBY_BIN "ruby"
#define DEFAULT_RDEBUG_BIN "/var/lib/gems/1.8/bin/rdebug"
#define DEFAULT_RDEBUG_ARGS (QStringList())
#endif



TDriverRunConsole::TDriverRunConsole(bool makeProc, QWidget *parent) :
    QWidget(parent),
    proc(makeProc ? new QProcess(this): NULL),
    toolbar(new QToolBar),
    layout(new QVBoxLayout),
    console(new TDriverConsoleTextEdit),
    outputMode(NO_OUTPUT)
{
    layout->setObjectName("runconsole");
    console->setObjectName("runconsole");

    console->setLocalEcho(true);
    createActions();

    toolbar->setObjectName("runconsole");
    toolbar->setObjectName("runconsole");
    toolbar->addActions(actions());
    toolbar->addActions(actions());
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    layout->addWidget(toolbar);
    layout->addWidget(console);

    setLayout(layout);

    if (proc) {
        proc->setProcessChannelMode(QProcess::MergedChannels);

        connect(proc, SIGNAL(error(QProcess::ProcessError)), this, SLOT(procError(QProcess::ProcessError)));
        connect(proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(procFinished(int,QProcess::ExitStatus)));
        connect(proc, SIGNAL(started()), this, SLOT(procStarted()));

        console->setIODevice(proc);
        // note: readProcText below is virtual, and may connect to a derived class method
        connect(proc, SIGNAL(readyRead()), this, SLOT(readProcText()));
    }
    //else qDebug() << FCFL << "started without proc";
}

TDriverRunConsole::~TDriverRunConsole()
{
    if (proc && proc->state() != QProcess::NotRunning) {
#ifdef Q_OS_WIN
        proc->kill();
#else
        proc->terminate();
#endif
    }
}


void TDriverRunConsole::createActions()
{
    clearAct = new QAction(QIcon(":/images/clear.png"), tr("&Clear"), this);
    clearAct->setObjectName("clear");
    clearAct->setToolTip(tr("Clear contents of buffer"));
    addAction(clearAct);
    connect(clearAct, SIGNAL(triggered()), console, SLOT(clear()));
    //clearAct->setEnabled(false);

    terminateAct = new QAction(QIcon(":/images/terminate.png"), tr("&Terminate process"), this);
    terminateAct->setObjectName("terminate");
    terminateAct->setToolTip(tr("terminate current process"));
    addAction(terminateAct);
    if (proc) {
        connect(terminateAct, SIGNAL(triggered()), proc, SLOT(terminate()));
#ifdef Q_OS_WIN
        // on Windows terminate means window close, and ruby scripts don't care about it
        // TODO: add option to enable terminate for scripts with GUI.
        terminateAct->setEnabled(false);
        terminateAct->setVisible(false);
#else
        connect(this, SIGNAL(runSignal(bool)), terminateAct, SLOT(setEnabled(bool)));
#endif
    }
    else {
        terminateAct->setVisible(false);
        terminateAct->setEnabled(false);
    }

    killAct = new QAction(QIcon(":/images/kill.png"), tr("&Kill process"), this);
    killAct->setObjectName("kill");
    killAct->setToolTip(tr("Kill current process"));
    addAction(killAct);
    if (proc) {
        connect(killAct, SIGNAL(triggered()), proc, SLOT(kill()));
        connect(this, SIGNAL(runSignal(bool)), killAct, SLOT(setEnabled(bool)));
    }
    else {
        killAct->setVisible(false);
        killAct->setEnabled(false);
    }
}


void TDriverRunConsole::clear()
{
    console->clear();
}


bool TDriverRunConsole::runFile(QString fileName, TDriverRunConsole::RunRequestType type)
{
    if (!proc) {
        qWarning() << FCFL << "called when proc==NULL";
        return false;
    }

    if (proc->state() != QProcess::NotRunning) {
        qDebug() << FCFL << "can't start a new process in process state" << proc->state();
        // TODO: add error message box
        return false;
    }

    disconnect(proc, SIGNAL(started()), this, SLOT(rdebugStarted()));
    emit needDebugConsole(type == Debug2Request);
    // debug consoles should be cleared by above signal
    clear();

    // TODO: names of programs should be configurable
    // note: indexes of progs array must match with integer values of RunRequestType

    QString progToRun;
    QStringList args;

    QFileInfo fi(fileName);
    QString scriptArg(fi.fileName());
    if (!fileName.isEmpty()) {
        proc->setWorkingDirectory(fi.absolutePath()); // script directory
    }
    else {
        proc->setWorkingDirectory(QDir::tempPath());
    }
    console->appendText(tr("Working directory: %1").arg(proc->workingDirectory()), console->notifyFormat);
    //static const QString RubyAutoflushPrefix("STDOUT.sync=true;");

    QSettings settings;
    QString RubyProg(settings.value("editor/ruby_application", DEFAULT_RUBY_BIN).toString());
    QString RdebugProg(settings.value("editor/rdebug_application", DEFAULT_RDEBUG_BIN).toString());
    QStringList RdebugArgs(settings.value("editor/rdebug_start_arguments", DEFAULT_RDEBUG_ARGS).toStringList());

    switch(type) {

    case RunRequest:
        outputMode = PROCESS_OUTPUT;
        progToRun = RubyProg;
        //args << "-e" << RubyAutoflushPrefix + "require '" + fileName + "'";
        //console->setLocalEcho(false);
        break;

    case Debug1Request:
#if TDRIVER_RUNCONSOLE_DEBUG1_ENABLED
        outputMode = PROCESS_OUTPUT;
        //progToRun = RubyProg;
        //args << "-e" << RubyAutoflushPrefix + "require 'ruby-debug'; debugger ; require '" + fileName + "'";
        progToRun = RdebugProg;
        args << Rdebug1stArg;
        //console->setLocalEcho(true);
        break;
#else
        qFatal("DEBUG1 function not enabled");
#endif

    case Debug2Request:
        outputMode = RDEBUG_OUTPUT;
        progToRun = RdebugProg;
        args << RdebugArgs;
        args << "-s"; // remote debugging server
        args << "-w"; // wait for connection
        // TODO: make these ports configurable
        args << "--port" << QString("%1").arg(remotePort);
        args << "--cport" << QString("%1").arg(controlPort);
        args << "--annotate" << "9";
        args << "--no-quit";
        connect(proc, SIGNAL(started()), this, SLOT(rdebugStarted()));
        //console->setLocalEcho(false);
        break;

    case InteractRequest:
        scriptArg = TDriverUtil::tdriverHelperFilePath(TDRIVER_RUBY_INTERACTSCRIPT);
        outputMode = PROCESS_OUTPUT;
        progToRun = RubyProg;
        //args << "-e" << RubyAutoflushPrefix + "require '" + fileName + "'";
        //console->setLocalEcho(false);
        break;

    default:
        qFatal("INVALID TYPE %i vs %i\n", type, InteractRequest);
    }

    args << scriptArg; // script filename
    qDebug() << FCFL << "EXEC: " << progToRun << "in dir" << proc->workingDirectory()
             << "with args " << args << "(modes" << type << outputMode << ")";
    console->appendLine("EXEC in " + proc->workingDirectory() + ": " + progToRun + " " + args.join(" "), console->notifyFormat);
    proc->start(progToRun, args, QIODevice::ReadWrite|QIODevice::Text);
    emit runSignal(true);

    return true;
}


void TDriverRunConsole::rdebugStarted()
{
    int msec = 1500;
    //emit lineForRdebugConsole(QString("xxDELAY BEFORE CONNECT: %1 ms").arg(msec));
    QTimer::singleShot(msec, this, SLOT(rdebugReady()));
}

void TDriverRunConsole::rdebugReady()
{
    if (outputMode == RDEBUG_OUTPUT) {
        //emit lineForRdebugConsole(QString("REQUESTING CONNECT: 127.0.0.1 ports %1 & %2").arg(remotePort).arg(controlPort));
        emit requestRemoteDebug("127.0.0.1", remotePort, controlPort, this);
    }
    else {
        qDebug() << FCFL << "Bad outputmode" << outputMode <<", script probably ended, signal ignored.";
#ifdef Q_OS_WIN
        QMessageBox::warning( this,
                             tr("Running script failed unexpectedly"),
                             tr("Check this:\n  Launch Windows Task Manager, then\n  kill 'ruby.exe' if it is running"));
#endif
    }
}

void  TDriverRunConsole::procError(QProcess::ProcessError err)
{
    if (proc) {
        qDebug() << FCFL << err << proc->errorString();
        console->appendLine(QString("process error reported: %1, %2").arg(err).arg(proc->errorString()), console->notifyFormat);
        emit runSignal(false);
    }
    else {
        qWarning() << FCFL << "called when proc==NULL!";
    }
}


void  TDriverRunConsole::procFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << FCFL << exitCode << exitStatus;
    QString msg;
    if (exitStatus==QProcess::NormalExit) {
        msg.setNum(exitCode);
        msg.prepend("PROCESS ENDED: clean exit with code ");
    }
    else {
        msg.setNum(exitCode, 16);
        msg = "PROCESS ENDED: abnormal exit with code 0x" + msg.toUpper();
    }
    console->appendLine(msg, console->notifyFormat);

    outputMode = NO_OUTPUT;
    emit runSignal(false);
    emit runningState(false);
}


void  TDriverRunConsole::procStarted(void)
{
    //qDebug() << FCFL << proc->bytesAvailable();
    console->appendLine("PROCESS STARTED", console->notifyFormat);
    emit runningState(true);
    Q_ASSERT(outputMode != NO_OUTPUT);
}

void TDriverRunConsole::endProcess(void)
{
    if (!proc) return;

#ifdef Q_OS_WIN
    proc->kill();
#else
    proc->terminate();
#endif
}

void TDriverRunConsole::setStdStreamsHidden(bool state)
{
    outputMode = state ? RDEBUG_OUTPUT : PROCESS_OUTPUT;
}


#if 0 // code not kept up to date, verify before enabling
void TDriverRunConsole::setStdStreamsVisible(bool state)
{
    outputMode = !state ? RDEBUG_OUTPUT : PROCESS_OUTPUT;
}
#endif


void TDriverRunConsole::readProcText()
{
    if (!proc) {
        qWarning() << FCFL << "called when proc==NULL!";
        return;
    }

    QByteArray rawData = proc->readAll();
    //    qDebug() << FCFL << "outputMode" << outputMode << "rawData:" << rawData;
    //    QString text(rawData);
    //    qDebug() << FCFL << "Process STDOUT/STDERR output:" << text.replace('\n', ".§.").replace('\r', ".§.");

    if (outputMode == PROCESS_OUTPUT) {
        console->appendText(rawData, console->outputFormat);
    }
    // else qDebug() << FFL << "discarded:" << QString::fromLocal8Bit(rawData);

    //procText(QString::fromLocal8Bit(rawData));
}

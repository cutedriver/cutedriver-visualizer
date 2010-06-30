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


#include "tdriver_consoletextedit.h"
#include "tdriver_runconsole.h"
#include "tdriver_debugconsole.h"
#include "tdriver_combolineedit.h"

#include <QVBoxLayout>
#include <QAction>
#include <QToolBar>
#include <QTcpSocket>
#include <QStringList>
#include <QMap>
#include <QRegExp>

#include <QLineEdit>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>

#include "tdriver_editor_common.h"

static const char rdebugDelimCstr[] = { 0x1A, 0x1A, 0 };

TDriverDebugConsole::TDriverDebugConsole(QWidget *parent) :
        QWidget(parent),
        remoteConn(new QTcpSocket(this)),
        controlConn(new QTcpSocket(this)),
        toolbar(new QToolBar),
        layout(new QVBoxLayout),
        remoteConsole(new TDriverConsoleTextEdit),
        controlConsole(new TDriverConsoleTextEdit),
        comboLine(new TDriverComboLineEdit()),
        cmdTargetCB(new QComboBox()),
        sendCmdButton(new QPushButton(tr("Send"))),
        remoteHasPrompt(false),
        controlHasPrompt(false),
        procConsoleOwner(NULL),
        isRunning(false),
        syncingBeforeContinue(false),
        dataSynced(false),
        quitWaiting(false),
        quitSent(false),
        interruptWaiting(false),
        interruptSent(false),
        rdebugDelim(rdebugDelimCstr),

        remoteParseKey(),
        remoteParsedLast(NULL),
        remoteParsedNew(new QMap<QString, QStringList>)
{
    remoteConsole->outputFormat.setForeground(QBrush(Qt::darkBlue));
    remoteConsole->commandFormat.setForeground(QBrush(Qt::blue));
    controlConsole->outputFormat.setForeground(QBrush(Qt::darkMagenta));
    controlConsole->commandFormat.setForeground(QBrush(Qt::magenta));

    layout->setObjectName("debugconsole");
    remoteConsole->setObjectName("debugconsole remote");
    controlConsole->setObjectName("debugconsole control");
    comboLine->setObjectName("debugconsole combo line");
    cmdTargetCB->setObjectName("debugconsole cmd target");
    sendCmdButton->setObjectName("debugconsole cmd send");

    remotePromptPrefix = rdebugDelim + "prompt";
    controlPromptPrefix = remotePromptPrefix;

    createActions();

    controlConsole->configureCommandLine(comboLine);
    controlConsole->disconnectCommandLine();
    remoteConsole->configureCommandLine(comboLine);
    remoteConsole->disconnectCommandLine();

    toolbar->setObjectName("debugconsole");
    toolbar->addActions(actions());
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    layout->addWidget(toolbar);

    layout->addWidget(remoteConsole);
    remoteConsole->setLocalEcho(true);

#if DEBUG_RDEBUG_CONTROL
    layout->addWidget(controlConsole);
    controlConsole->setLocalEcho(true);
#elif 0
    controlConsole->setQuiet(true);
#else
    // redirect controlConsole output to remoteConsole
    controlConsole->copyAppendCursor(remoteConsole);
    //controlConsole->setAppendCursorColor(Qt::red);
#endif

    {
        QHBoxLayout *sublayout = new QHBoxLayout;
        sublayout->setObjectName("debugconsole cmd");
        cmdTargetCB->insertItem(0, tr("rdebug"));
        cmdTargetCB->insertItem(1, tr("control"));

        sublayout->addWidget(comboLine);
        sublayout->addWidget(sendCmdButton);
        sublayout->addWidget(cmdTargetCB);

        connect(comboLine, SIGNAL(triggered(QString)), sendCmdButton, SLOT(click()));
        connect(sendCmdButton, SIGNAL(clicked()), this, SLOT(cmdToDebugger()));
        layout->addLayout(sublayout);
        remoteConsole->setReadOnly(true);
        controlConsole->setReadOnly(true);
    }

    remoteConsole->setIODevice(remoteConn);
    controlConsole->setIODevice(controlConn);
    connect(remoteConn, SIGNAL(readyRead()), this, SLOT(readRemoteText()));
    connect(controlConn, SIGNAL(readyRead()), this, SLOT(readControlText()));

    remoteConsole->setReadOnly(true);
    controlConsole->setReadOnly(true);
    setLayout(layout);
}


void TDriverDebugConsole::clear()
{
    remoteConsole->clear();
    controlConsole->clear();
}


void TDriverDebugConsole::resetConnections()
{
    if (remoteConn->isOpen()) {
        remoteConn->reset();
        qDebug() << FCFL << "remoteConn reseted";
    }

    if (controlConn->isOpen())  {
        controlConn->reset();
        qDebug() << FCFL << "controlConn reseted";
    }
}


void TDriverDebugConsole::connectTo(QString host, quint16 dport, quint16 cport, TDriverRunConsole *procConsoleOwner_)
{
    resetConnections();

    procConsoleOwner = procConsoleOwner_;

    //qDebug() << FCFL << host << dport << cport;
    remoteConsole->appendLine(
            QString("Connecting to ruby-debug at %1 ports %2 and %3").arg(host).arg(dport).arg(cport),
            remoteConsole->notifyFormat);

    bool ok = false;
    remoteConn->connectToHost(host, dport, QIODevice::ReadWrite | QIODevice::Text);
    ok = remoteConn->waitForConnected(2000);

    if (!ok) {
        qWarning() << "rdebug remote connect timeout/error:" << remoteConn->error();
    }
    else {
        controlConn->connectToHost(host, cport, QIODevice::ReadWrite | QIODevice::Text);
        ok = controlConn->waitForConnected(2000);
        if (!ok) {
            qWarning() << "rdebug control connect timeout/error:" << controlConn->error();
        }
    }

    if (!ok) {
        resetConnections();
    }

    quitWaiting = false;
    updateActionStates();
    emit connectResult(ok);
}


void TDriverDebugConsole::setRunning(bool state)
{
    //qDebug() << FCFL << state << "---------------------------";
    isRunning = state;

    if (!isRunning) resetConnections();

    remoteCmdQueue.clear();
    controlCmdQueue.clear();

    dataSynced =
            syncingBeforeContinue =
            controlHasPrompt =
            remoteHasPrompt =
            quitSent =
            interruptSent =
            quitWaiting =
            interruptWaiting
            = false;

    updateActionStates();
}


void TDriverDebugConsole::createActions()
{
    clearAct = new QAction(QIcon(":/images/clear.png"), tr("&Clear"), this);
    clearAct->setObjectName("debugconsole clear");
    clearAct->setToolTip(tr("Clear contents of buffer"));
    connect(clearAct, SIGNAL(triggered()), remoteConsole, SLOT(clear()));

    quitAct = new QAction(QIcon(":/images/quit.png"), tr("&Quit"), this);
    quitAct->setObjectName("debugconsole quit");
    quitAct->setToolTip(tr("Step Into (rdebug 'step')"));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(doQuit()));

    stepIntoAct = new QAction(QIcon(":/images/stepinto.png"), tr("Step &Into"), this);
    stepIntoAct->setObjectName("debugconsole step into");
    stepIntoAct->setToolTip(tr("Interrupt and quit script"));
    connect(stepIntoAct, SIGNAL(triggered()), this, SLOT(doStepInto()));

    stepOverAct = new QAction(QIcon(":/images/stepover.png"), tr("Step &Over"), this);
    stepOverAct->setObjectName("debugconsole step over");
    stepOverAct->setToolTip(tr("Step Over (rdebug 'next')"));
    connect(stepOverAct, SIGNAL(triggered()), this, SLOT(doStepOver()));

    // note: interrupt and continue button labels are updated elsewhere,
    // the text set below should never become visible

    interruptAct = new QAction(QIcon(":/images/interrupt.png"), "(interrupt)", this);
    interruptAct->setObjectName("debugconsole interrupt");
    interruptAct->setToolTip(tr("Interrupt script"));
    connect(interruptAct, SIGNAL(triggered()), this, SLOT(doInterrupt()));

    continueAct = new QAction(QIcon(":/images/continue.png"), "(continue)", this);
    continueAct->setObjectName("debugconsole continue");
    continueAct->setToolTip(tr("Start/continue script"));
    connect(continueAct, SIGNAL(triggered()), this, SLOT(doContinue()));

    addAction(interruptAct);
    addAction(continueAct);
    addAction(stepIntoAct);
    addAction(stepOverAct);
    addAction(clearAct);
    addAction(quitAct);
}


void TDriverDebugConsole::updateActionStates()
{
    bool remoteActsOk = isRunning && remoteConn->isOpen() && remoteHasPrompt;
    continueAct->setEnabled(remoteActsOk);
    stepIntoAct->setEnabled(remoteActsOk);
    stepOverAct->setEnabled(remoteActsOk);
    sendCmdButton->setEnabled(remoteActsOk);

    sendCmdButton->setEnabled(isRunning);
    //qDebug() << FCFL << isRunning << controlConn->isOpen() << controlHasPrompt;

    quitAct->setEnabled(isRunning && !quitWaiting && remoteConn->isOpen());
    interruptAct->setEnabled(isRunning && !remoteHasPrompt && !interruptSent && !interruptWaiting && controlConn->isOpen());


    if (!isRunning || remoteHasPrompt) continueAct->setIconText(tr("Continue"));
    else continueAct->setIconText(tr("...running"));

    if (!isRunning || !remoteHasPrompt)    interruptAct->setIconText(tr("Interrupt"));
    else interruptAct->setIconText(tr("...interrupted"));

    // some simple font effects
    {
        QFont font = interruptAct->font();
        bool state = isRunning && interruptSent;
        font.setItalic(state);
        font.setUnderline(state);
        interruptAct->setFont(font);
    }
    {
        QFont font = quitAct->font();
        bool state = isRunning && (quitSent || (quitWaiting && interruptSent));
        font.setItalic(state);
        font.setUnderline(state);
        quitAct->setFont(font);
    }
}

void TDriverDebugConsole::doQuit(void)
{
    if (remoteHasPrompt) {
        remoteConn->write("quit unconditionally\n");
        remoteConsole->appendLine("quit", remoteConsole->commandFormat);
        remoteHasPrompt = false;
        quitWaiting = false;
        quitSent = true;
    }
    else {
        quitWaiting = true;
        doInterrupt();
    }
    updateActionStates();
}

void TDriverDebugConsole::doInterrupt(void)
{
    if (interruptSent) return;

    if (controlHasPrompt /*&& !remoteHasPrompt*/) {
        controlConn->write("interrupt\n");
        controlConsole->appendLine("interrupt", controlConsole->commandFormat);
        interruptSent = !remoteHasPrompt;
        controlHasPrompt = false;
        interruptWaiting = false;
    }
    else  {
        interruptWaiting = true;
    }
    updateActionStates();
}


void TDriverDebugConsole::cmdToDebugger()
{
    if (comboLine->currentText().isEmpty()) return;

    if (cmdTargetCB->currentIndex()==0) { // remote
        //qDebug() << FCFL << "remoteHasPrompt" << remoteHasPrompt;
        if (!remoteHasPrompt) return;
        sendRemoteCmd(comboLine->currentText());
    }
    else { // control
        //qDebug() << FCFL << "controlHasPrompt" << controlHasPrompt;
        if (!controlHasPrompt) return;
        sendControlCmd(comboLine->currentText());
    }
    comboLine->clearEditText();
}

static bool sendCmd(bool &hasPrompt, QStringList &queue, QTcpSocket *connection, TDriverConsoleTextEdit *console, QString &cmd, bool allowQueueing)
{
    //qDebug() << FCFL << QString(cmd.isNull()?"<NULL>":cmd) << allowQueuing << "(hasPrompt" << hasPrompt << "queue" << queue.size() << ")";
    if (hasPrompt) {
        if (!queue.isEmpty()) {
            if (!cmd.isNull()) queue.append(cmd);
            cmd = queue.takeFirst();
        }
        if (!cmd.isNull()) {
            qDebug() << FFL << "WRITE" << cmd;
            connection->write((cmd+"\n").toLocal8Bit());
            console->appendLine(cmd, console->commandFormat);
            hasPrompt = false;
        }
        return true;
    }
    else if (allowQueueing) {
        queue.append(cmd);
        return true;
    }
    else
        return false;

}

bool TDriverDebugConsole::sendRemoteCmd(QString cmd, bool allowQueuing)
{
    bool res = sendCmd(remoteHasPrompt, remoteCmdQueue, remoteConn, remoteConsole, cmd, allowQueuing);
    if (res) updateActionStates();
    if (!res) qDebug() << "REMOTE COMMAND NOT GOING TO BE SENT: " << cmd;
    return res;
}


bool TDriverDebugConsole::sendControlCmd(QString cmd, bool allowQueuing)
{
    bool res = sendCmd(controlHasPrompt, controlCmdQueue, controlConn, controlConsole, cmd, allowQueuing);
    if (res) updateActionStates();
    if (!res) qDebug() << "CONTROL COMMAND NOT GOING TO BE SENT: " << cmd;
    return res;
}


void TDriverDebugConsole::doContinue(void)
{
    sendRemoteCmd("continue");
}

void TDriverDebugConsole::doStepInto(void)
{
    sendRemoteCmd("step");
}

void TDriverDebugConsole::doStepOver(void)
{
    sendRemoteCmd("next");
}


static inline void remoteParsedDone(QMap<QString, QStringList> *&target, QMap<QString, QStringList> *&tmp)
{
    if (target) delete target;
    target = tmp;
    tmp = new QMap<QString, QStringList>;
    //qDebug() << "got parselist" << *target;
}

static inline void dumpToConsole(TDriverConsoleTextEdit *console, QMap<QString, QStringList> *map)
{
    foreach(QString key, map->keys()) {
        qDebug() << FFL << "-----------------" << key;
        if (map->value(key).isEmpty())
            console->appendLine(rdebugDelimCstr + key + ": <no values>", console->notifyFormat);
        else
            console->appendLine(rdebugDelimCstr + key + ": '" + map->value(key).join("', '") + "'", console->notifyFormat);
    }
}

static QList<struct MEC::Breakpoint> parseBreakpoints(QStringList list)
{
    static const QRegExp rx("^(\\d+) +([yn]) +at +(.+):(\\d+)$");
    //obsolete parsing from control: "^Breakpoint (\\d+) file (.+), line (\\d+)$");

    QList<struct MEC::Breakpoint> result;
    if (list.isEmpty()) return result;

    QString firstline(list.takeFirst());
    if (list.isEmpty() || firstline != QObject::tr("Num Enb What", "rdebug header for breakpoint list printout")) {
        if (firstline != QObject::tr("No breakpoints.", "rdebug printout for no breakpoints")) {
            qWarning() << FFL << "UNEXPECTED OUTPUT FROM rdebug, PLEASE REPORT A TICKET";
            qDebug() << "breakpoint list:" << list;
        }
        return result;
    }

    foreach (QString line, list) {
        if (rx.indexIn(line) == -1 || rx.capturedTexts().size() != 1+4) {
            qWarning() << FFL << "Invalid breakpoint line ignored:" << line;
            qDebug() << FFL << "captures:" << rx.capturedTexts()
                    << "pattern:" << rx.pattern();
            continue;
        }

        bool ok1, ok4;
        struct MEC::Breakpoint bp = {
            num:rx.cap(1).toInt(&ok1),
                enabled:(rx.cap(2) == "y"),
                    file:rx.cap(3),
                        line:rx.cap(4).toInt(&ok4) };
        if (ok1 && ok4)
            result.append(bp);
        else
            qDebug() << FFL << "invalid parse result" << ok1 << ok4 << "for line" << line;
    }

    qDebug() << FFL << MEC::dumpBreakpointList(result, "\n  ", "\n  ");
    return result;
}

static inline bool checkForPrompt(const QString &str, const QString &prefix)
{
    //return str.startsWith(prefix);
    return (str == prefix);
}

void TDriverDebugConsole::handleRemoteText(QString text)
{
    remoteHasPrompt = false;

    remoteBuffer.append(text);
    QStringList spl = remoteBuffer.split('\n', QString::KeepEmptyParts);
    Q_ASSERT(!spl.isEmpty());
    remoteBuffer = spl.takeLast(); // either partial line or empty string

    qDebug() << FCFL << "REMOTE\n" << spl.join("\n") << "\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^";
    foreach(QString str, spl) {

        // handle "PROMPT ..pre-prompt" line so it won't be displayed as application output
        if (str == "PROMPT "+rdebugDelim+"pre-prompt") {
            str = rdebugDelim+"pre-prompt";
        }

        qDebug() << FCFL << str << remoteParseKey;

        if (checkForPrompt(str, remotePromptPrefix)) {
            // got prompt, we're back in control
            if (!remoteHasPrompt) qDebug() << FCFL << "REMOTE GOT PROMPT by" << str;
            remoteHasPrompt = true;

            if (!dataSynced) {
                qDebug() << FCFL << "emit requestDataSync";
                emit requestDataSync();
                dataSynced = true;
            }

            // any output to script console will be from rdebug
            // TODO: if there are threads, maybe setStdStreamsHidden shouldn't be done?
            if (procConsoleOwner) procConsoleOwner->setStdStreamsHidden(true);

            qDebug() << FCFL << "GOT PARSELIST remoteParsed2" << *remoteParsedNew;
            remoteParsedDone(remoteParsedLast, remoteParsedNew);
            remoteParseKey.clear();
            //dumpToConsole(remoteConsole, remoteParsed2);

            //foreach(const QString &line, remoteParsedLast->value("breakpoints"))
                //remoteConsole->appendLine("BREAKPOINT "+line, remoteConsole->outputFormat);
            foreach(const QString &line, remoteParsedLast->value("stack"))
                remoteConsole->appendLine("STACK"+line, remoteConsole->outputFormat);
            foreach(const QString &line, remoteParsedLast->value("variables"))
                remoteConsole->appendLine(line, remoteConsole->outputFormat);
            foreach(const QString &line, remoteParsedLast->value("error-begin"))
                remoteConsole->appendLine("ERROR: "+line, remoteConsole->outputFormat);

            emit breakpoints(parseBreakpoints(remoteParsedLast->value("breakpoints")));
            emitRunningPosition(*remoteParsedLast, false);

            interruptSent = false;
            if (quitWaiting) doQuit();

            sendRemoteCmd(); // handle remoteCmdQueue
            //            if (remoteHasPrompt) {
            //                remoteConsole->appendText("> ", remoteConsole->commandFormat);
            //            }
        }
        else if (str.startsWith(rdebugDelim)) {
            // got  new key for parsed information items
            qDebug() << FCFL << remoteParseKey << "chaged to" << str.mid(rdebugDelim.length()).trimmed();
            remoteParseKey = str.mid(rdebugDelim.length()).trimmed();
            // make sure a map key exists even if there won't be any data associated with it
            if (!remoteParseKey.isEmpty()) (*remoteParsedNew)[remoteParseKey];

            // "starting" means script is running and rdebug remote will not be responsive
            if (remoteParseKey == "starting") {
                remoteParsedNew->clear();
                if (procConsoleOwner) procConsoleOwner->setStdStreamsHidden(false);
            }
        }
        else if (!remoteParseKey.isEmpty()) {
            // got new information item for current key
            (*remoteParsedNew)[remoteParseKey].append(str.trimmed());
        }
        else {
            remoteConsole->appendLine(str, remoteConsole->outputFormat);
        }
    }
    updateActionStates();
}


void TDriverDebugConsole::handleControlText(QString text)
{
    controlBuffer.append(text);
    QStringList spl = controlBuffer.split('\n', QString::KeepEmptyParts);
    controlBuffer = spl.takeLast(); // either partial line or empty string

    qDebug() << FCFL << "CONTROL" << spl;
    foreach(QString str, spl) {
        //controlConsole->appendLine(str, controlConsole->outputFormat);
        // handle "PROMPT ..pre-prompt" line so it won't be displayed as application output
        if (str == "PROMPT "+rdebugDelim+"pre-prompt") {
            str = rdebugDelim+"pre-prompt";
        }

        static bool notifyAboutControlPrompt = true;
        if (checkForPrompt(str, controlPromptPrefix)) {

            if (!controlHasPrompt) {
                //qDebug() << FCFL << "control GOT prompt";
                if (notifyAboutControlPrompt) {
                    controlConsole->appendLine("control ready", controlConsole->notifyFormat);
                    notifyAboutControlPrompt = false;
                }
                controlHasPrompt = true;
            }
            if (interruptWaiting) doInterrupt();
            sendControlCmd(); // handle remoteCmdQueue
        }
        else {
            if (controlHasPrompt) {
                qDebug() << FCFL << "control lost prompt unexpectedly with text" << str;
                controlConsole->appendLine("control waiting", controlConsole->notifyFormat);
                notifyAboutControlPrompt = true;
                controlHasPrompt = false;
            }
            if (str.startsWith(rdebugDelim) || str.startsWith("(rdb:ctrl)")) {
                qDebug() << FCFL << "hidden output:" << str;
            }
            else {
                controlConsole->appendLine(str, controlConsole->outputFormat);
            }
        }
    }
    updateActionStates();
}


void TDriverDebugConsole::resetProcConsoleOwner()
{
    procConsoleOwner = NULL;
}


void TDriverDebugConsole::readRemoteText()
{
    QByteArray rawData = remoteConn->readAll();
    //qDebug() << FCFL << "RAW REMOTE" << rawData << "=" << rawData.toHex();
    handleRemoteText(QString::fromLocal8Bit(rawData));
}


void TDriverDebugConsole::readControlText()
{
    QByteArray rawData = controlConn->readAll();
    //qDebug() << FCFL << "RAW CONTROL" << rawData << "=" << rawData.toHex();
    //static const char promptStr[] = "PROMPT (rdb:ctrl) ";
    //QByteArray printData(rawData);
    //printData.replace(promptStr, "");
    //controlConsole->appendText(printData, controlConsole->outputFormat);

    handleControlText(QString::fromLocal8Bit(rawData));
}


void TDriverDebugConsole::emitRunningPosition(QMap<QString, QStringList> &remoteParsed, bool starting)
{
    static const QString currentLineStart("--> #0 ");
    static const QString parseStart("at line ");

    QStringList stack(remoteParsed.value("stack"));

    //QString function; // emited value // TODO: implement reporting function, add to signal arguments
    QString entry;

    if (stack.isEmpty()) return;
    QString line0(stack.takeFirst());
    if (!line0.startsWith(currentLineStart)) return;
    if (!stack.isEmpty()) {
        QString line1 = stack.takeFirst();
        if (line1.startsWith(parseStart)) {
            // 2-line format get function from line0 and at_line from line1
            //function = line0.mid(currentLineStart.length());
            entry = line1.mid(parseStart.length());
        }
    }
    if (entry.isEmpty()) {
        // 1-line format, function will stay empty
        int ind = line0.indexOf(parseStart, currentLineStart.length());
        if (ind == -1) return;
        entry = line0.mid(ind + parseStart.length());
    }

    // try to find colon preceding line number
    int colonInd = entry.lastIndexOf(':');
    if (colonInd < 0) return;

    // try to parse line number
    bool ok = false;
    int lineNum = entry.mid(colonInd+1).toInt(&ok); // emited vaule
    if (!ok)  return;

    QString fileName = entry.left(colonInd); // emitted value
    if (fileName.isEmpty()) return;

    fileName = MEC::absoluteFilePath(fileName);
    if (starting) {
        qDebug() << FCFL << "emitting runStarting(" << fileName << lineNum <<")";
        emit runStarting(fileName, lineNum);
    }
    else {
        qDebug() << FCFL << "emitting gotRemotePrompt(" << fileName << lineNum <<")";
        emit gotRemotePrompt(fileName, lineNum);
    }
}


void TDriverDebugConsole::addBreakpoint(struct MEC::Breakpoint bp)
{
    if (!isRunning) {
        qDebug() << FCFL << "not running, ignored";
        return;
    }
    Q_ASSERT(bp.num <= 0);
    Q_ASSERT(bp.enabled == true);
    QString cmd(QString("break %1:%2").arg(bp.file).arg(bp.line));
    sendRemoteCmd(cmd, true);
    //    if (sendRemoteCmd(cmd, true)) qDebug() << FCFL << "Sent to remote:" << cmd;
    //else qDebug() << FCFL << "Remote busy. Command not sent:" << cmd;
}


void TDriverDebugConsole::removeBreakpoint(int rdebugInd)
{
    if (!isRunning) {
        qDebug() << FCFL << "not running, ignored";
        return;
    }
    QString cmd(QString("delete %1").arg(rdebugInd));
    sendRemoteCmd(cmd, false);
    //    if (sendRemoteCmd(cmd)) qDebug() << FCFL << "Sent to remote:" << cmd;
    //else qDebug() << FCFL << "Remote busy. Command not sent:" << cmd;
}


void TDriverDebugConsole::removeAllBreakpoints()
{
    if (!isRunning) {
        qDebug() << FCFL << "not running, ignored";
        return;
    }

    qDebug() << FCFL << "\"delete\"";
    sendControlCmd("delete", true);
}

void TDriverDebugConsole::allSynced()
{
    if (syncingBeforeContinue) {
        syncingBeforeContinue = false;
        sendRemoteCmd("continue", true);
    }
}

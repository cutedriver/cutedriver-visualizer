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


#include "tdriver_rubyinteract.h"
#include "tdriver_consoletextedit.h"
#include <QVBoxLayout>
#include <QAction>
#include <QIcon>
#include <QToolBar>

#include "tdriver_editor_common.h"

static const char delimChar = 032; // note: octal

static const char InteractDelimCstr[] = { delimChar, delimChar, 0 };


TDriverRubyInteract::TDriverRubyInteract(QWidget *parent) :
        TDriverRunConsole(parent),
        isRestarting(false),
        havePrompt(false),
        waitingEnd(false),
        //state(CLOSED),
        delimStr(InteractDelimCstr),
        promptId(delimStr + "PROMPT:"),
        argsId(delimStr + "ARGPROMPT:"),
        errorId(delimStr + "ERROR:"),
        infoId(delimStr + "INFO:"),
        evalId(delimStr + "EVAL:"),
        endId(delimStr + "END:"),
        //argStr(delimStr + "ARG:"),
        outputFormat(new QTextCharFormat),
        waitingEndOfRecord(false)
{
    outputFormat->setForeground(QBrush(Qt::darkGray));
    outputFormat->setFontFixedPitch(true);
    layout->setObjectName("irconsole");
    console->setObjectName("irconsole");

    createActions();

    //disconnect(proc, SIGNAL(readyRead()), 0, 0);
    //console->setIODevice(proc, true);
    connect(proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(checkRestart(int,QProcess::ExitStatus)));
    //setStdStreamsHidden(false);
}


void TDriverRubyInteract::restart()
{
    restartWithQueue(false);
}

void TDriverRubyInteract::restartWithQueue(bool keepQueryQueue)
{
    if (isRunning()) {
        if (keepQueryQueue) {
            qWarning() << FCFL << "Warning: restarting running process, noClear ignored";
        }
        endProcess();
        isRestarting = true;
        return;
    }

    if (!keepQueryQueue) {
        queryQueue.clear();
    }
    runFile(activeDocName, TDriverRunConsole::InteractRequest);
}


void TDriverRubyInteract::checkRestart(int, QProcess::ExitStatus)
{
    if (isRestarting) {
        isRestarting = false;
        restart();
    }
}


void TDriverRubyInteract::setActiveDocumentName(QString name)
{
    activeDocName = name;
}


void TDriverRubyInteract::createActions()
{
    QList <QAction*> acts;

    restartAct = new QAction(QIcon(":/images/restart.png"), tr("&Restart"), this);
    restartAct->setObjectName("restart");
    restartAct->setToolTip(tr("Clear contents of buffer"));
    acts.append(restartAct);
    connect(restartAct, SIGNAL(triggered()), this, SLOT(restart()));

    insertActions(actions().first(), acts);
    toolbar->insertActions(toolbar->actions().first(), acts);

}


/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

void  TDriverRubyInteract::procStarted(void)
{
    havePrompt = waitingEnd = waitingEndOfRecord = false;
    readBuffer.clear();
    resultLines.clear();
    TDriverRunConsole::procStarted();
    //state = READ_PROMPT;
}


bool TDriverRubyInteract::sendNextQuery()
{
    if (queryQueue.isEmpty()) return true; // TODO: or false? check if return value is actually used ever

    if (!proc->isOpen()) {
        qDebug() << FCFL "Script not running, doing restart";
        restartWithQueue();
        return true;
    }

    else if (proc->isWritable() && havePrompt) {
        struct QueryQueueItem query = queryQueue.first();

        int lines = query.statement.isEmpty() ? 0
            : query.statement.count('\n')+1;
        QString cmd = QString("%1 %2\n").arg(query.command).arg(lines);

        if (lines > 0)
            cmd.append(query.statement+"\n");
        proc->write(cmd.toLocal8Bit());
        waitingEnd = true;
        qDebug() << FCFL << "wrote" << cmd;
        havePrompt = false;
        return true;
    }

    else {
        qDebug() << FCFL << "Unable to write at the moment (writable" << proc->isWritable() << ", prompt" << havePrompt << "waitingEndOfRecord" << waitingEndOfRecord << ")";
        return false;
    }
}


bool TDriverRubyInteract::queryCompletions(QString statement)
{
    struct QueryQueueItem query;
    query.type = QueryQueueItem::COMPLETION,
    query.client = sender(),
    query.command = "line_completion",
    query.statement = statement.trimmed();

    queryQueue.append(query);
    return sendNextQuery();
}


bool TDriverRubyInteract::evalStatement(QString statement)
{
    struct QueryQueueItem query;
    query.type = QueryQueueItem::EVALUATION,
    query.client = sender(),
    query.command = "line_execution",
    query.statement = statement.trimmed();

    queryQueue.append(query);
    return sendNextQuery();
}


void TDriverRubyInteract::readProcText()
{
    // note: this slot gets connected to in parent class constructor

    QByteArray rawData = proc->readAll();
    //qDebug() << FCFL << "outputMode" << outputMode << "rawData:" << rawData;
    {
        //QString text(rawData);
        //qDebug() << FCFL << "Interact script output:" << text.replace('\n', ".§.").replace('\r', ".§.");
    }

    if (outputMode == PROCESS_OUTPUT) {
        console->appendText(rawData, *outputFormat);
    }

    readBuffer.append(rawData);
    QStringList spl = readBuffer.split('\n', QString::KeepEmptyParts);
    //qDebug() << FCFL << spl << spl.size();
    readBuffer = spl.takeLast(); // next line, either empty or partial

    foreach(QString line, spl) {
        //qDebug() << FCFL << line;
        line = line.trimmed();

        if (!waitingEndOfRecord) {
            if (line.startsWith(delimStr)) {

                waitingEndOfRecord = true;
                // TODO: these if's should just note start of status record, and handling should be in if(waintingEndOfRecord... below
                if (line.startsWith(promptId)) {
                    qDebug() << FCFL << "PROMPT line:" << line;
                    havePrompt = true;
                    resultLines.clear();
                    sendNextQuery();
                }

                else if (line.startsWith(errorId)) {
                    if (waitingEnd) {
                        Q_ASSERT(!queryQueue.isEmpty());
                        struct QueryQueueItem query = queryQueue.takeFirst();
                        waitingEnd = false;

                        emit scriptError(query.client, query.statement, resultLines);
                    }
                }

                else if (line.startsWith(endId)) {
                    if (waitingEnd) {
                        Q_ASSERT(!queryQueue.isEmpty());
                        struct QueryQueueItem query = queryQueue.takeFirst();
                        waitingEnd = false;

                        switch (query.type) {
                        case QueryQueueItem::COMPLETION:
                            emit completionResult(query.client, query.statement, resultLines);
                            break;
                        case QueryQueueItem::EVALUATION:
                            emit evalutaionResult(query.client, query.statement, resultLines);
                            break;
                        }
                    }
                }

                else if (line.startsWith(infoId)) {
                    qDebug() << FCFL << "INFO line:" << line;
                }

                else if (line.startsWith(evalId)) {
                    qDebug() << FCFL << "EVAL line:" << line;
                }

                else if (line.startsWith(argsId)) {
                    qDebug() << FCFL << "ARGPROMPT line:" << line;
                }

                else {
                    qDebug() << FCFL << "ignoring line" << line;
                }
            }
            else {
                // regular output
                Q_ASSERT(!waitingEndOfRecord);
                //qDebug() << FCFL << resultLines << "<<" << line;
                resultLines.append(line);
            }
        }

        // code above may have set waitingEndOfRecord for current line
        if (waitingEndOfRecord && line.endsWith(delimChar)) {
            //if (!line.startsWith(delimStr)) qDebug() << FCFL << "\t" << line;
            waitingEndOfRecord = false;
        }
    }
}

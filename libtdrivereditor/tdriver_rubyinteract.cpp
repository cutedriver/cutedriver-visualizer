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
#include <QLabel>
#include <QMessageBox>

#include "tdriver_editor_common.h"

#include <tdriver_rubyinterface.h>

#include <tdriver_debug_macros.h>
#include <tdriver_util.h>


TDriverRubyInteract::TDriverRubyInteract(QWidget *parent) :
        TDriverRunConsole(false, parent), // false means QProcess will not be created
        stdoutFormat(new QTextCharFormat),
        stderrFormat(new QTextCharFormat),
        prevSeqNum(0)
{
    commandLineLabel->setText(tr("Eval:"));

    stdoutFormat->setForeground(QBrush(Qt::darkGray));
    stdoutFormat->setFontFixedPitch(true);

    stderrFormat->setForeground(QBrush(Qt::darkRed));
    stderrFormat->setFontFixedPitch(true);

    layout->setObjectName("irconsole");
    console->setObjectName("irconsole");

    createActions();

    connect(TDriverRubyInterface::globalInstance(), SIGNAL(rubyProcessFinished()),
            this, SLOT(resetQueryQueue()));

    connect(TDriverRubyInterface::globalInstance(), SIGNAL(rubyOnline()), this, SLOT(rubyIsOnline()));

    connect(TDriverRubyInterface::globalInstance(), SIGNAL(rubyOutput(int, quint32,QByteArray)),
            this, SLOT(rbiText(int, quint32,QByteArray)));

    connect(TDriverRubyInterface::globalInstance(), SIGNAL(messageReceived(quint32,QByteArray,BAListMap)),
            this, SLOT(rbiMessage(quint32,QByteArray,BAListMap)));

    // used for progressing in query queue after receiving a reply
    connect(this, SIGNAL(requestNextQuery()), this, SLOT(sendNextQuery()), Qt::QueuedConnection);
}


void TDriverRubyInteract::resetQueryQueue()
{
    while ( !queryQueue.empty() ) {
        QueryQueueItem query = queryQueue.takeFirst();

        switch (query.type) {
        case QueryQueueItem::COMPLETION:
            qDebug() << FCFL << "completionError for statement:" << query.statement;
            emit completionError(query.client, query.statement, QStringList());
            break;
        case QueryQueueItem::EVALUATION:
            qDebug() << FCFL << "evaluationnError for statement:" << query.statement;
            emit evaluationError(query.client, query.statement, QStringList());
            break;
        }
    }
}


static QString joinLines(const BAList &lines)
{
    QString ret;
#if 0
    int reserve = 1;
    for (int ii=0; ii<lines.count(); ++ii) {
        reserve += lines.at(ii).size()+1;
    }
    ret.reserve(reserve);
#endif
    for (int ii=0; ii<lines.count(); ++ii) {
        ret.append(lines.at(ii));
        if (ii) ret.append('\n');
    }
    return ret;
}


void TDriverRubyInteract::resetScript()
{
    qDebug() << FCFL;

    resetQueryQueue();
    prevSeqNum = 0;

    BAListMap msg;
    bool ok = TDriverRubyInterface::globalInstance()->executeCmd("interact reset", msg, 5000, "reset");

    if (ok) {
        if (msg.contains("error")) {
            qWarning() << FCFL << "ERROR FROM SCRIPT" << msg;
            QMessageBox::warning(this, tr("Ryby Reset error"),
                                 tr("Sent command 'interact reset'.\nGot error:\n%1")
                                 .arg(joinLines(msg.value("error"))));

        }
        else {
            qDebug() << FCFL << "success";
            QMessageBox::information(this, tr("Ruby instance reseted"),
                                 tr("Successfully created a new instance for interactive Ruby evaluation."));
        }
    }
    else {
        qDebug() << FCFL << "executeCmd returned false";
        TDriverRubyInterface::globalInstance()->requestClose();
        QMessageBox::warning(this, tr("Ruby reset time-out"),
                             tr("Reset command time-out,\nrequested closing Ruby process."));
    }
}


void TDriverRubyInteract::createActions()
{
    QList <QAction*> acts;

    resetAct = new QAction(QIcon(":/images/reset.png"), tr("&Reset"), this);
    resetAct->setObjectName("reset");
    resetAct->setToolTip(tr("Clear contents of buffer"));
    acts.append(resetAct);
    connect(resetAct, SIGNAL(triggered()), this, SLOT(resetScript()));

    insertActions(actions().first(), acts);
    toolbar->insertActions(toolbar->actions().first(), acts);

}


/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

void  TDriverRubyInteract::procStarted(void)
{
    qDebug() << FCFL;
    console->appendLine("PROCESS STARTED", console->notifyFormat);
    //state = READ_PROMPT;
}


void TDriverRubyInteract::rubyIsOnline()
{
    qDebug() << FCFL;
}


bool TDriverRubyInteract::sendNextQuery()
{
    QString goOnlineError;

    if (queryQueue.isEmpty()) {
        return true;
    }

    else if (!(goOnlineError = TDriverRubyInterface::globalInstance()->goOnline()).isNull()) {
        return false;
    }

    else {
        struct QueryQueueItem &query = queryQueue.first();

        if (query.rbiSeqNum == 0) {
            BAListMap msg;
            msg["command"] << query.command << query.statement;
            query.rbiSeqNum = TDriverRubyInterface::globalInstance()->sendCmd("interaction", msg);
            if (query.rbiSeqNum == 0) {
                qDebug() <<FCFL << ">>>> sendCmd returned failure, command remains in queryQueue, size" << queryQueue.size();
                return false;
            }
            else {
                qDebug() <<FCFL << ">>>> sendCmd returned seqnum" << query.rbiSeqNum;
                return true;
            }
        }
        else {
            qDebug() << FCFL << "Unable to send command at the moment, with queryQueue size"<< queryQueue.size();
            return false;
        }
    }
}


bool TDriverRubyInteract::queryCompletions(QByteArray statement)
{
    struct QueryQueueItem query;
    query.type = QueryQueueItem::COMPLETION,
    query.client = sender(),
    query.rbiSeqNum = 0;
    query.command = "line_completion",
    query.statement = statement.trimmed();

    queryQueue.append(query);
    return sendNextQuery();
}


bool TDriverRubyInteract::evalStatement(QByteArray statement)
{
    struct QueryQueueItem query;
    query.type = QueryQueueItem::EVALUATION,
    query.client = sender(),
    query.rbiSeqNum = 0;
    query.command = "line_execution",
    query.statement = statement.trimmed();

    queryQueue.append(query);
    return sendNextQuery();
}


void TDriverRubyInteract::rbiMessage(quint32 seqNum, QByteArray name, BAListMap message)
{
    qDebug() << FCFL << "<<<<<<<<<<" << seqNum << name;
    if (queryQueue.isEmpty()) return; // no pending queries

    struct QueryQueueItem &query = queryQueue.first();

    if (seqNum < query.rbiSeqNum) return; // ignore

    // after this point, received message affects queryQueue even if it's not for us

    if (seqNum == query.rbiSeqNum && name == "interaction") {
        // it's for us!

        switch (query.type) {

        case QueryQueueItem::COMPLETION:
            {
                QStringList completionLines;
                foreach (QByteArray key, message["result_keys"]) {
                    foreach(QByteArray line, message[key]) {
                        completionLines << QString::fromLocal8Bit(line.constData(), line.size());
                    }
                }
                qDebug() << FCFL << completionLines;
                emit completionResult(query.client, query.statement, completionLines);
            }
            break;

        case QueryQueueItem::EVALUATION:
            qDebug() << FCFL << "EVALUATION RESULT" << message;
            //emit evaluationResult(query.client, query.statement, resultLines); // sent by rbiStdoutText/rbiStderrText
            emit evaluationResult(query.client, query.statement, QStringList()); // sent by rbiStdoutText/rbiStderrText
            break;

        }
    }
    else {
        qDebug() << FCFL << "name" << name << "with received seqNum" << seqNum << "vs. removed queue seqNum" << query.rbiSeqNum;
        // NOTE: assert below assumes overlapping queries won't be send, so all except first item in queue have seqNum 0
        Q_ASSERT(queryQueue.length() < 2 || queryQueue.at(1).rbiSeqNum == 0);
    }
    prevSeqNum = query.rbiSeqNum;
    queryQueue.removeFirst();
    if (!queryQueue.isEmpty()) emit requestNextQuery();
}


bool TDriverRubyInteract::checkOutputSeqNum(quint32 seqNum)
{
    if (seqNum == 0) return false;
    else if (seqNum == prevSeqNum) return true;
    else return ( !queryQueue.isEmpty() && seqNum == queryQueue.first().rbiSeqNum);
}


void TDriverRubyInteract::rbiText(int fnum, quint32 seqNum, QByteArray text)
{
    if ( checkOutputSeqNum(seqNum) ) {
        if (fnum == 0)
            console->appendText(QString::fromLocal8Bit(text.constData(), text.size()), *stdoutFormat);

        else if (fnum == 1)
            console->appendText(QString::fromLocal8Bit(text.constData(), text.size()), *stderrFormat);

        else qDebug() << FCFL << "bad fnum" << fnum << seqNum << text;
    }
    // else not for us
}

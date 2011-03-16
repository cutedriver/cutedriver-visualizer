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


#ifndef TDRIVER_RUBYINTERACT_H
#define TDRIVER_RUBYINTERACT_H

#include <QWidget>
#include <QList>

#include "tdriver_runconsole.h"

#include <tdriver_rubyinterface.h>


class QTextCharFormat;

class TDriverRubyInteract : public TDriverRunConsole
{
Q_OBJECT
public:
    explicit TDriverRubyInteract(QWidget *parent = 0);


signals:
    void completionResult(QObject *client, QByteArray statement, QStringList result);
    void completionError(QObject *client, QByteArray statement, QStringList result);

    void evaluationResult(QObject *client, QByteArray statement, QStringList result);
    void evaluationError(QObject *client, QByteArray statement, QStringList result);

    void requestNextQuery();

public slots:
    void resetQueryQueue();
    void resetScript();
    void rubyIsOnline();
    bool sendNextQuery();
    bool queryCompletions(QByteArray statement);
    bool evalStatement(QByteArray statement);

protected slots:
    virtual void procStarted(void); // interited from RunConsole

    void rbiMessage(quint32 seqNum, QByteArray name, BAListMap message);
    void rbiText(int fnum, quint32 seqNum, QByteArray text);

protected:
    void createActions();

    QAction *resetAct;
    QAction *restartAct;

    struct QueryQueueItem {
        enum QueryType { COMPLETION, EVALUATION };
        QueryType type;
        QObject *client;
        quint32 rbiSeqNum;
        QByteArray command;
        QByteArray statement;
    };

    QList<struct QueryQueueItem> queryQueue;

private:
    bool checkOutputSeqNum(quint32 seqNum);

private:
    bool isReady;
    QTextCharFormat *stdoutFormat;
    QTextCharFormat *stderrFormat;

    quint32 prevSeqNum; // used for accepting STDOUT/STDERR text coming after message text
};

#endif // TDRIVER_RUBYINTERACT_H

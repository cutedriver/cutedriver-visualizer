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

#include "tdriver_runconsole.h"

#include <QList>

class QTextCharFormat;

class TDriverRubyInteract : public TDriverRunConsole
{
Q_OBJECT
public:
    //enum InteractState { CLOSED, READ_PROMPT, READ_CMD, READ_ARG, READ_DATA };

    explicit TDriverRubyInteract(QWidget *parent = 0);

signals:
    void completionResult(QObject *client, QString statement, QStringList result);
    void evalutaionResult(QObject *client, QString statement, QStringList result);
    void scriptError(QObject *client, QString statement, QStringList result);

public slots:
    void restart();
    void restartWithQueue(bool keepQueryQueue = true);
    void checkRestart(int,QProcess::ExitStatus);
    void setActiveDocumentName(QString);
    bool sendNextQuery();
    bool queryCompletions(QString statement);
    bool evalStatement(QString statement);

protected slots:
    virtual void procStarted(void);
    virtual void readProcText();

protected:
    void createActions();

    QAction *restartAct;

    bool isRestarting;
    QString activeDocName;
    //InteractState state;
    QString readBuffer;
    QStringList resultLines;
    bool havePrompt;
    bool waitingEnd;

    struct QueryQueueItem {
        enum { COMPLETION, EVALUATION } type;
        QObject *client;
        QString command;
        QString statement;
    };

    QList<struct QueryQueueItem> queryQueue;

    const QString delimStr;
    const QString promptId;
    const QString argsId;
    const QString errorId;
    const QString infoId;
    const QString evalId;
    const QString endId;

private:
    QTextCharFormat *outputFormat;
    bool waitingEndOfRecord;
};

#endif // TDRIVER_RUBYINTERACT_H

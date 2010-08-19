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

#ifndef TDRIVER_RUBYINTERFACE_H
#define TDRIVER_RUBYINTERFACE_H

#include "libtdriverutil_global.h"

#include "tdriver_rbiprotocol.h"

#include <QThread>
#include <QProcess>
#include <QAbstractSocket>

class QMutex;
class QWaitCondition;

class LIBTDRIVERUTILSHARED_EXPORT TDriverRubyInterface : public QThread
{
Q_OBJECT
public:
    explicit TDriverRubyInterface();
    ~TDriverRubyInterface();

    static void startGlobalInstance();
    void requestClose();
    static TDriverRubyInterface *globalInstance();

    bool goOnline();

    bool executeCmd( const QByteArray &name, BAListMap &cmd_reply, unsigned long timeout);
    quint32 sendCmd( const QByteArray &name, const BAListMap &cmd);

    int getPort();
    int getRbiVersion();
    QString getTDriverVersion();

    void setValidThread(QThread *id) { validThread = id; }

protected:
    void run();

signals:
    void error(QString title, QString text, QString details);
    void requestRubyConnection();
    void requestCloseSignal();
    void rubyProcessFinished();
    void rubyOnline();
    void rubyOffline();
    void messageReceived(quint32 seqNum, QByteArray name, BAListMap message);

    void rubyOutput(int fnum, QByteArray line);
    void rubyOutput(int fnum, quint32 seqNum, QByteArray text);

public slots:
    //bool running();
    void readProcessStdout();
    void readProcessStderr();

private slots:
    void close();
    void resetRubyConnection();
    void recreateConn();
    void resetProcess();
    void recreateProcess();
    //void messageFromHandler(quint32 seqNum, QByteArray name, BAListMap message);

private:
    void readProcessHelper(int fnum, QByteArray &readBuffer, quint32 &seqNum, QByteArray &evalBuffer);

private:
    int rbiPort;
    int rbiVersion;
    QString rbiTDriverVersion;

    // these are created by this class, but deleted by other
    QMutex *syncMutex;
    QWaitCondition *msgCond;
    QWaitCondition *helloCond;

    QProcess *process;
    QAbstractSocket *conn;
    TDriverRbiProtocol *handler;

    static TDriverRubyInterface *pGlobalInstance;

    enum { Closed, Running, Connected, Closing } initState;
    QString initErrorMsg;


    QByteArray stderrBuffer;
    QByteArray stdoutBuffer;
    QByteArray stderrEvalBuffer;
    QByteArray stdoutEvalBuffer;
    quint32 stderrEvalSeqNum;
    quint32 stdoutEvalSeqNum;

    const QByteArray delimStr;
    const QByteArray evalStartStr;
    const QByteArray evalEndStr;

    QThread *validThread;

};


#endif // TDRIVER_RUBYINTERFACE_H


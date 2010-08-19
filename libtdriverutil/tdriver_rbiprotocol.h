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

#ifndef TDRIVER_RBIPROTOCOL_H
#define TDRIVER_RBIPROTOCOL_H

// RBI stands for Ruby Interface

#include "libtdriverutil_global.h"

#include <QObject>

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QAbstractSocket>

class QMutex;
class QWaitCondition;
class QThread;

class LIBTDRIVERUTILSHARED_EXPORT TDriverRbiProtocol : public QObject
{
Q_OBJECT
public:
    explicit TDriverRbiProtocol(QAbstractSocket *connection, QMutex *cm, QWaitCondition *mwc, QWaitCondition *hwc, QObject *parent = 0);
    ~TDriverRbiProtocol();

    quint32 nextSeqNum() { return nextSN; }
    BAListMap waitedMessage() { return condMsg; }

    void setValidThread(QThread *id) { validThread = id; }

    const BAListMap &helloMessage() const { return helloMsg; }

    bool isHelloReceived() const { return haveHello; }

public:
    bool waitHello(unsigned long timeout);
    bool waitSeqNum(quint32 seqNum, unsigned long timeout);
    static BAList parseList(const QByteArray &data);
    static BAListMap parseListMap(const QByteArray &data);
    static void makeStringListMapMsg(QByteArray &target, const QByteArray &name, const BAListMap &msg, quint32 seqNum);

signals:
    // for inter-thread communication, internal use only
    void writeDataReady(QByteArray data);
    void messageReceived(quint32 seqNum, QByteArray name, BAListMap message);
    void helloReceived();
    void gotDisconnection();

public slots:
    void connected();
    void readyToRead();
    void bytesWritten(qint64 bytes);
    void disconnected();
    void connError(QAbstractSocket::SocketError);

    quint32 sendStringListMapMsg(const QByteArray &name, const BAListMap &map, quint32 seqNum=0);
#if 0
    void dumpReceivedMessage(quint32 seqNum, QByteArray name, QByteArray data);
    void bounceReceivedMessage(quint32 seqNum, QByteArray name, QByteArray data);
#endif

private slots:
    void resetMessage();
    void addWriteData(QByteArray data);

private:
    enum { ReadDisconnected, ReadSeqNum, ReadNameLen, ReadName, ReadDataLen, ReadData } readState;
    qint32 readAmount;
    QByteArray readBuffer;
    quint32 receivedSN;
    QByteArray currentName;
    QByteArray currentData;

    QByteArray writeBuffer;

    QAbstractSocket *conn;
    BAListMap helloMsg;

    QMutex *condMutex;
    QWaitCondition *cond;

    quint32 condSeqNum;
    BAListMap condMsg;
    QByteArray condName;

    quint32 nextSN;
    bool haveHello;
    QWaitCondition *helloCond;

    QThread *validThread;
};

#endif // TDRIVER_RBIPROTOCOL_H

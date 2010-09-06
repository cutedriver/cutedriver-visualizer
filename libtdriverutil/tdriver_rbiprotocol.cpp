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


#include "tdriver_rbiprotocol.h"
// RBI stands for Ruby Interface

#include <QCoreApplication>
#include <QDataStream>
#include <QAbstractSocket>
#include <QHostAddress>

#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QThread>

#include "tdriver_debug_macros.h"

// debug macros
#define VALIDATE_THREAD (Q_ASSERT(validThread == NULL || validThread == QThread::currentThread()))
#define VALIDATE_THREAD_NOT (Q_ASSERT(validThread != QThread::currentThread()))


TDriverRbiProtocol::TDriverRbiProtocol(QAbstractSocket *connection, QMutex *cm, QWaitCondition *mwc, QWaitCondition *hwc, QObject *parent) :
    QObject(parent),
    readState(ReadDisconnected),
    conn(connection),
    syncMutex(cm),
    msgCond(mwc),
    nextSN(0),
    haveHello(false),
    helloCond(hwc),
    validThread(NULL)

{
    helloMsg.clear();

    condSeqNum = 0;
    condName.clear();
    condMsg.clear();

    connect(conn, SIGNAL(connected()), this, SLOT(connected()));
    connect(conn, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(conn, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connError(QAbstractSocket::SocketError)));
    connect(conn, SIGNAL(readyRead()), this, SLOT(readyToRead()));
    connect(conn, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
    //connect(conn, SIGNAL(disconnected()), QCoreApplication::instance(), SLOT(quit()));

    connect(this, SIGNAL(writeDataReady(QByteArray)),
            this, SLOT(addWriteData(QByteArray)),
            Qt::QueuedConnection);
}


TDriverRbiProtocol::~TDriverRbiProtocol()
{
    qDebug() << FCFL << "(destructor)";
}



void TDriverRbiProtocol::resetMessage()
{
    readState = ReadSeqNum;
    readAmount = sizeof(quint32);
    readBuffer.clear();
}

void TDriverRbiProtocol::connected()
{
    qDebug() << FCFL << "to" << conn->peerAddress() << conn->peerPort();
    VALIDATE_THREAD;

    condSeqNum = 0;
    condName.clear();
    condMsg.clear();

    resetMessage();
    writeBuffer.clear();
    haveHello = false;
}


void TDriverRbiProtocol::disconnected()
{
    VALIDATE_THREAD;
    if (readState != ReadDisconnected) {
        qDebug() << FCFL << "Disconnected, UNREAD OUTPUT:" << conn->readAll();
        readState = ReadDisconnected;
        conn->close();
        emit gotDisconnection();
    }
    else qDebug() << FCFL << "no action for readState" << readState;
}


void TDriverRbiProtocol::connError(QAbstractSocket::SocketError err)
{
    VALIDATE_THREAD;
    if (readState != ReadDisconnected) {
        qDebug() << FCFL << err << "UNREAD OUTPUT:" << conn->readAll();
        readState = ReadDisconnected;
        conn->close();
        emit gotDisconnection();
    }
    else {
        qDebug() << FCFL << err << "no action for readState" << readState;
    }
}


void TDriverRbiProtocol::addWriteData(QByteArray data)
{
    VALIDATE_THREAD;
    writeBuffer.append(data);
    qint64 written = conn->write(writeBuffer);
    if (written > 0) {
        writeBuffer.remove(0, written);
    }
}


void TDriverRbiProtocol::makeStringListMapMsg(QByteArray &target, const QByteArray &name, const BAListMap &msg, quint32 seqNum)
{
    QDataStream msgStream(&target, QIODevice::Append | QIODevice::WriteOnly);

    msgStream << seqNum;
    msgStream << name;

    QByteArray mapBuf;
    {
        QDataStream mapStream(&mapBuf, QIODevice::WriteOnly | QIODevice::Append);

        BAListMap::const_iterator  mapIter;
        for (mapIter = msg.constBegin(); mapIter != msg.constEnd(); ++mapIter) {
            mapStream << mapIter.key();

            QByteArray listBuf;
            {
                QDataStream listStream(&listBuf, QIODevice::WriteOnly | QIODevice::Append);
                foreach(QByteArray listItem, mapIter.value()) {
                    listStream << listItem;
                }
            }
            mapStream << listBuf;
        }
    }
    msgStream << mapBuf;
}


quint32 TDriverRbiProtocol::sendStringListMapMsg(const QByteArray &name, const BAListMap &msg, quint32 seqNum)
{
    if (seqNum == 0) {
        if (nextSN == 0) seqNum = 1;
        else seqNum = nextSN;
    }
    nextSN = seqNum + 1;
    if (nextSN < seqNum) seqNum = 1, nextSN=2; // in case this ever wraps around...

    //qDebug() << FCFL << "SENDING" << seqNum << name << "\n>>>>>>" << msg;

    QByteArray wBuf;
    makeStringListMapMsg(wBuf, name, msg, seqNum);
    emit writeDataReady(wBuf);

    return seqNum;
}


BAList TDriverRbiProtocol::parseList(const QByteArray &data)
{
    BAList list;
    QDataStream listStream(data);
    forever {
        QByteArray strData;
        listStream >> strData;
        if (listStream.status() != QDataStream::Ok) break;
        list.append(strData);
    }
    return list;
}


BAListMap TDriverRbiProtocol::parseListMap(const QByteArray &data)
{
    BAListMap map;

    QDataStream mapStream(data);
    forever {

        QByteArray keyData;
        QByteArray listData;
        mapStream >> keyData;
        if (mapStream.status() != QDataStream::Ok) break;

        mapStream >> listData;
        if (mapStream.status() != QDataStream::Ok) {
            qWarning() << FFL << "got map item with key" << keyData << "but without data!";
            break;
        }
        map[keyData] = parseList(listData);
    }

    return map;
}


void TDriverRbiProtocol::readyToRead()
{
    //qDebug() << FCFL << "ENTRY";
    VALIDATE_THREAD;

    do {
        while (readAmount > readBuffer.size() && conn->isReadable() && conn->bytesAvailable() > 0) {
            // TODO: have timeout here, in case server works incorrectly.
            // This code assumes that server always writes as many bytes of data as it says
            readBuffer += conn->read(readAmount - readBuffer.size());
        }

        if (readBuffer.size() < readAmount) continue; // avoids readBuffer being cleared
        Q_ASSERT(readAmount == readBuffer.size());

        bool messageOver = false;

        switch (readState) {

        case ReadSeqNum:
            {
                QDataStream parseStream(readBuffer);
                parseStream >> receivedSN;
            }
            readAmount = sizeof(quint32);
            readState = ReadNameLen;
            break;

        case ReadNameLen:
            {
                QDataStream parseStream(readBuffer);
                parseStream >> readAmount;
                //qDebug() << FFL << "got nameLen" << readAmount;
            }
            if (readAmount == 0) {
                readState = ReadDisconnected;
            }
            else {
                readState = ReadName;
            }
            break;

        case ReadName:
            currentName = readBuffer;
            //qDebug() << FFL << "got name" << currentName;
            readAmount = sizeof(quint32);
            readState = ReadDataLen;
            break;

        case ReadDataLen:
            {
                QDataStream parseStream(readBuffer);
                parseStream >> readAmount;
            }
            if (readAmount == 0) {
                //qDebug() << FFL << "got dataLen" << 0;
                currentData.clear();
                messageOver = true;
            }
            else {
                readState = ReadData;
                readBuffer.reserve(readAmount);
            }
            break;
        case ReadData:
            currentData = readBuffer;
            //qDebug() << FFL << "got data bytes" << currentData.size();
            messageOver = true;
            break;

        case ReadDisconnected:
            conn->disconnectFromHost();
            break;
        }

        if (messageOver) {
            syncMutex->lock();

            condSeqNum = receivedSN;

            if (nextSN <= receivedSN) nextSN = receivedSN+1;
            condName = currentName;

            condMsg = parseListMap(currentData);
            if (condName == "hello") {
                // handle hello message specially
                haveHello = true;
                helloMsg = condMsg;
                qDebug() << FCFL << "Received HELLO";
                helloCond->wakeAll();
                emit helloReceived();
            }
            else {
                //qDebug() << FCFL << "RECEIVED" << condSeqNum << condName << "\n<<<<<<" << condMsg;
                msgCond->wakeAll();
                emit messageReceived(condSeqNum, condName, condMsg);
            }
            syncMutex->unlock();

            resetMessage(); // also clears readBuffer
        }
        else readBuffer.clear(); // readBuffer never contains more than one data element, so clear it after handling of each element
        // if readAmount was not read, 'continue' was used earlier, and readBuffer won't get cleared

    } while (conn->bytesAvailable() > 0);
}


void TDriverRbiProtocol::bytesWritten(qint64 written)
{
    //qDebug() << FFL << "written" << written << ", current writeBuffer size" << writeBuffer.size();
    VALIDATE_THREAD;

    if (conn->isWritable() && !writeBuffer.isEmpty()) {
        // write more!
        written = conn->write(writeBuffer);
        writeBuffer.remove(0, written);
        //qDebug() << FFL << "more written" << written << ", remaining writeBuffer size" << writeBuffer.size();
    }

    if (written == 0 && !writeBuffer.isEmpty()) {
        qWarning() << FFL << "writeBuffer not empty after 0 bytes written";
    }
}


bool TDriverRbiProtocol::waitHello(unsigned long timeout)
{
    VALIDATE_THREAD_NOT;
    return (haveHello)
            ? true
                : helloCond->wait(syncMutex, timeout);
}

bool TDriverRbiProtocol::waitSeqNum(quint32 seqNum, unsigned long timeout)
{
    qDebug() << FFL << "for seqNum" << seqNum;
    VALIDATE_THREAD_NOT;

    // handler.condMutex must be locked when entering here!
    if (!syncMutex || !msgCond) return false;

    forever {
        // test timeout

        if (!msgCond->wait(syncMutex, timeout)) {
            qDebug() << FCFL << "TIMEOUT waiting for seqNum" << seqNum;
            return false;
        }
        // test for success (seqNum 0 accepts any sequence number)
        if (seqNum == 0 || condSeqNum == seqNum) return true;

        // test missed seqNum
        if (condSeqNum == 0 || condSeqNum > seqNum) return false;
    }
}


#if 0
void TDriverRbiProtocol::dumpReceivedMessage(quint32 seqNum, QByteArray name, QByteArray data)
{
    qDebug() << FFL << "seqnum" << seqNum << "name" << name << "datalen" << data.length();
}
void TDriverRbiProtocol::bounceReceivedMessage(quint32 seqNum, QByteArray name, QByteArray data)
{
    qDebug() << FFL << "seqnum" << seqNum << "name" << name << "datalen" << data.length();
    BAListMap map(parseListMap(data));
    qDebug() << FFL << "map" << map;
    sendStringListMapMsg(seqNum+1, name, map);
}
#endif

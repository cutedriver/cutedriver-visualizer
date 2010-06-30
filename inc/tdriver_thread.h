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
 
 

#ifndef TDRIVERRUBYTHREAD_H
#define TDRIVERRUBYTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QProcess>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QRegExp>

#include <QtGui/QApplication>
#include <QtGui/QMessageBox>

//#include <QtCore/QMutex>


class RubyThread : public QThread {

    Q_OBJECT

public:

    RubyThread( QObject *parent = 0 );
    ~RubyThread();

    bool start();
    bool running();
    void close();

    bool offlineMode;

    bool execute_cmd( QString cmd );
    bool execute_cmd( QString cmd, QString& errorMessage );

private:

    bool criticalErrorShownAlready;

    QProcess process;

    bool listenerNotFound;

};

#endif


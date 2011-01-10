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



#include "tdriver_main_window.h"

#include <tdriver_util.h>

#include <QDateTime>

static QFile *debugOutFile;

static void output(QtMsgType type, const char *msg)
{
    if (msg == NULL || *msg == 0) return;

    // ?? if (message.contains("Event type")) return;

    QByteArray msgBuf;
    msgBuf.reserve(strlen(msg)+20);

    switch ( type ) {

        case QtDebugMsg:
            msgBuf.append("Debug: ");
            break;

        case QtWarningMsg:
            msgBuf.append("Warning: ");
            break;

        case QtCriticalMsg:
            msgBuf.append("Critical: ");
            break;

        case QtFatalMsg:
            msgBuf.append("Fatal: ");
            break;
    }

    msgBuf.append( msg );
    if (!msgBuf.endsWith('\n'))
        msgBuf.append('\n');

    debugOutFile->write(msgBuf);
    debugOutFile->flush();
}


int main(int argc, char *argv[])
{

    QApplication app(argc, argv);
    debugOutFile = new QFile(QDir::tempPath() + "/tdriver_visualizer_main.log" );

    // This causes QProcessManager instance to be started, which prevents deadlock in Qt 4.7.2+
    QProcess::execute(QString("."));

    // ignore errors in remove and rename
    QFile::remove(debugOutFile->fileName()+".1");
    QFile::rename(debugOutFile->fileName(), debugOutFile->fileName()+".1");

    debugOutFile->open( QIODevice::WriteOnly | QIODevice::Text );
    qInstallMsgHandler(output);
    qDebug("%s", qPrintable("Log opened: " + QDateTime::currentDateTime().toString()));

    qRegisterMetaType<BAList>("BAList");
    qRegisterMetaType<BAListMap>("BAListMap");

    MainWindow* mainWindow = new MainWindow();

    if ( mainWindow->setup() ){

        mainWindow->show();
        return app.exec();

    } else {

        // errors during setup
        return 1;

    }

}

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

QFile *out;

void output(QtMsgType type, const char *msg)
{

    QString message( msg );

    if ( message.length() <= 0 ) { return; }

    if ( message.contains( "Event type" ) ) { return; }

    out->write("Visualizer<>");

    switch ( type ) {

        case QtDebugMsg:
            out->write("Debug: ");
            break;

        case QtWarningMsg:
            out->write("Warning: ");
            break;

        case QtCriticalMsg:
            out->write("Critical: ");
            break;

        case QtFatalMsg:
            out->write("Fatal: ");
            break;

    }

    out->write( msg );
    out->write( "\n" );

    out->flush();

}


int main(int argc, char *argv[])
{

    QApplication app(argc, argv);
    out = new QFile ( QString( QApplication::applicationDirPath() ) + "/tdriver_visualizer.log" );
    out->open( QIODevice::WriteOnly | QIODevice::Text );
    qInstallMsgHandler(output);

    MainWindow* mainWindow = new MainWindow();

    mainWindow->setApplicationPath();

    if ( mainWindow->setup() ){

        mainWindow->show();
        return app.exec();

    } else {

        // errors during setup
        return 1;

    }

}

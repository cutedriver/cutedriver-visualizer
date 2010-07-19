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

#include "tdriver_thread.h"
#include <QApplication>
#include <tdriver_util.h>


RubyThread::~RubyThread(){}


RubyThread::RubyThread( QObject *parent ) :
        QThread( parent )
{
    listenerNotFound = false;
    criticalErrorShownAlready = false;
    start();
}


bool RubyThread::start()
{

    QString msgBoxTitle = "";
    QString msgBoxContent = "";
    QString errorMessage = "";
    bool execute_status = false;

    // set RUBYOPT env. setting if system doesn't have it already
    QStringList envSettings = QProcess::systemEnvironment();
    if ( QString( getenv( "RUBYOPT" ) ) != "rubygems" ) { envSettings << "RUBYOPT=rubygems"; }
    process.setEnvironment( envSettings );
    // if TDRIVER_VISUALIZER_LISTENER environment variable is set, use a custom file to use as the listener
    QString listener = TDriverUtil::tdriverHelperFilePath("listener.rb", "TDRIVER_VISUALIZER_LISTENER");

    offlineMode = false;

    if ( !QFile::exists( listener ) && !listenerNotFound ) {
        execute_status = false;
        listenerNotFound = true;
        msgBoxTitle = "Failed to initialize TDriver";
        msgBoxContent = QString( "Could not find Visualizer listener server file:\n\n%1" ).arg(listener);

    } else {
        process.start( "ruby", QStringList() << listener );

        if ( process.waitForStarted( 30000 ) ) {
            qDebug( "RubyThread: Process started succesfully" );
            execute_status = true;

        } else {
            qDebug( "RubyThread: Failed to start Ruby thread" );
            msgBoxTitle = "Failed to start Ruby thread";
            msgBoxContent =
                    QString("Please verify that Ruby interpreter is installed.\n\n") +
                    QString("PATH environment setting must contain folder where Ruby executable is installed.  ");
            execute_status = false;

        }

        if ( execute_status ) {

            // wait one minute to get ready signal
            if ( process.waitForReadyRead( 60000 ) ) {

                qDebug( "RubyThread: Got ready to read signal" );
                QByteArray resultByteArray = process.readAllStandardOutput();
                errorMessage = QString( resultByteArray.data() );
                qDebug() << "RubyThread: Received:" << QString( resultByteArray.data() ).replace( QRegExp( "\n" ), "\\n" );

                //if done string found in result all is ok
                if ( !resultByteArray.contains( "done" ) ) {

                    execute_status = false;
                    msgBoxTitle = "Failed to initialize TDriver";

                    if ( resultByteArray.contains( "LoadError: tdriver GEM" ) ) {
                        QString rubyoptString = getenv( "RUBYOPT" );
                        msgBoxContent =
                                QString( "Please verify that tdriver GEM is installed properly.  " ) +
                                QString( rubyoptString.contains( "rubygems", Qt::CaseInsensitive ) ? "" : "\n\nNote: RUBYOPT environment setting must be set to 'rubygems'  " );

                    } else {

                        msgBoxContent =
                                QString( "Error occured while initializing TDriver.\nSee details for more information.\n\nIf the problem persists, please contact support.  " );

                    }
                }

            } else {
                msgBoxTitle = "Failed to initialize TDriver";
                msgBoxContent = "Error reading data from listener server process";
                execute_status = false;

            }
        }
    }

    if ( !execute_status ) {

        //msgBoxContent += QString("\n\nNote that reading/sending data from device will not work.  ");
        msgBoxContent += QString("\n\nNote that communicating with SUT will not work. \nLaunching in offline mode.");

        if ( !criticalErrorShownAlready ) {

            QMessageBox msgBox;
            msgBox.setIcon( QMessageBox::Critical );
            msgBox.setWindowTitle( msgBoxTitle );
            msgBox.setText( msgBoxContent );
            msgBox.setSizeGripEnabled(true);

            if ( !errorMessage.isEmpty() ) {
                msgBox.setDetailedText( errorMessage );
            }

            msgBox.setStandardButtons( QMessageBox::Ok );
            msgBox.exec();

        }

        offlineMode = true;
    }

    return execute_status;
}


bool RubyThread::running()
{
    bool result = false;

    if ( process.state() == QProcess::Starting ) {
        // if timeout exceeds, return false
        result = process.waitForStarted( 60000 ) && process.waitForReadyRead( 60000 );

    } else {
        result = ( process.state() == QProcess::Running );

    }

    return result;
}


void RubyThread::close()
{
    qDebug( "RubyThread: Closing process" );
    process.write( "quit\n" );
    process.kill();
}


bool RubyThread::execute_cmd( QString cmd )
{
    QString errorMessage = "";
    return execute_cmd( cmd, errorMessage );
}


bool RubyThread::execute_cmd( QString cmd, QString & errorMessage )
{

    bool execute_status = false;
    errorMessage = "";
    QString tmp_cmd = cmd + "\n";

    if ( listenerNotFound || offlineMode ) {
        errorMessage = "TDriver or listener server is not running. Please verify that TDriver is installed properly and restart Visualizer.  \n\nIf the problem persists, please contact support.\n";
        return false;
    }

    if ( !running() ) {
        qDebug() << "RubyThread: Warning: Ruby thread not running anymore! Restaring listener server process...";
        start();
    }

    // qDebug( "[RubyThread::execute_cmd] writing command to process" );

    process.write( tmp_cmd.toStdString().c_str() );
    qDebug() << "RubyThread: Sent:" << QString( cmd );
    QString errorStr = "An error occured while executing command: \"" + cmd + "\".\n";

    //wait one minute to get ready signal
    if ( process.waitForReadyRead( 60000 ) ) {
        //qDebug( "RubyThread: Got ready to read signal" );
        QByteArray resultByteArray = process.readAllStandardOutput();
        qDebug() << "RubyThread: Received:" << QString( resultByteArray.data() ).replace( QRegExp( "[\n|\r]" ), "\\n" );

        // if done string found in result all is ok
        if ( resultByteArray.contains( "done" ) ) {
            execute_status = true;
            errorMessage = QString( resultByteArray.data() ).toLatin1();

        } else {
            errorStr = errorStr + "\n" + resultByteArray.data();

        }
    }
    /* else {
        qDebug() << "RubyThread: readyRead() signal was not emitted within 60 seconds";
        if ( !running() ) {
            qDebug() << "RubyThread: Warning: Ruby thread not running anymore! Restaring Ruby process...";
            start();
        }
    } */

    if ( !execute_status ) {

        qDebug() << "RubyThread: An error occured while executing command:" << QString( cmd );
        //an error occured / did not get ready to read, trying to read all standard output if a real error occured");

        QByteArray resultByteArray = process.readAllStandardOutput();
        //QByteArray resultErrorByteArray = process.readAllStandardError();

        qDebug() << "RubyThread: Received:" << QString( resultByteArray.data() ).replace( QRegExp( "\n" ), "\\n" );

        //qDebug() << "RubyThread: Received:" << QString( process.errorString() ); //#resultErrorByteArray.data() ).replace( QRegExp( "\n" ), "\\n" );

        //QString errorMsg = "";
        //errorMsg = resultByteArray.data();
        //errorMsg = errorMsg + resultErrorByteArray.data();

        if ( !resultByteArray.isEmpty() ) {
            //there was something more
            errorStr = errorStr + "\n" + "Additional error message: " + resultByteArray.data();
        }

        errorMessage = errorStr;
    }

    return execute_status;
}

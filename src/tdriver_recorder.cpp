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



#include <QtCore/QFile>

#include <QtGui/QTextDocument>
#include <QtGui/QTextBlock>
#include <QtGui/QProgressDialog>
#include <QtGui/QMessageBox>

#include "tdriver_recorder.h"

/*!
    \class TDriverRecorder
    \brief Uses TDriver to record script fragments from testable applications.

    TDriverRecorder uses TDriver framework to record scripts from testable qt
    applications. Certain mouse events are tracked and test script fragment
    is generated from them. Ouputs the fragment to an editor window.

 */

TDriverRecorder::TDriverRecorder( QWidget* parent ) :
        QDialog( parent )
{
    setup();
}

TDriverRecorder::~TDriverRecorder() {
}

void TDriverRecorder::setOutputPath(  QString path ) {

    outputPath = path;

}

void TDriverRecorder::setActiveDevAndApp( QString strActiveDevice, QString activeApp ) {

    mStrActiveDevice = strActiveDevice;
    mActiveApp = activeApp;

}

void TDriverRecorder::setup() {

    resize( 800,500 );

    setWindowTitle( "TDriver Visualizer Recorder" );

    mScriptField = new QTextEdit( this );
    mScriptField->setObjectName("recorder text");

    mScriptField->setLineWrapMode( QTextEdit::NoWrap );
    mScriptField->setEnabled( false );

    mRecButton = new QPushButton( tr( "&Record" ) );
    mRecButton->setObjectName("recorder rec");

    mStopButton = new QPushButton( tr( "&Stop" ) );
    mStopButton->setObjectName("recorder stop");
    mStopButton->setEnabled( false );

    mTestButton = new QPushButton( tr( "&Test" ) );
    mTestButton->setObjectName("recorder test");
    mTestButton->setEnabled( false );

    QPushButton* close = new QPushButton( tr( "&Close" ) );
    close->setObjectName("recorder close");

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setObjectName("recorder buttons");

    layout->addWidget( mRecButton );
    layout->addWidget( mStopButton );
    layout->addWidget( mTestButton );
    layout->addWidget( close );

    connect( mRecButton, SIGNAL( clicked() ), this, SLOT( startRecording() ) );
    connect( mStopButton, SIGNAL( clicked() ), this, SLOT( stopRecording() ) );
    connect( mTestButton, SIGNAL( clicked() ), this, SLOT( testRecording() ) );

    connect( close, SIGNAL( clicked() ), this, SLOT( hide() ) );

    QGridLayout* grid = new QGridLayout();
    grid->setObjectName("recorder");

    grid->addWidget( mScriptField, 0, 0 );
    grid->addLayout( layout, 1, 0 );

    setLayout( grid );

    setSizeGripEnabled( true );
}


void TDriverRecorder::setActionsEnabled(bool start, bool stop, bool test)
{
    mRecButton->setEnabled( start );
    mStopButton->setEnabled( stop );
    mTestButton->setEnabled( test );
}


void TDriverRecorder::startRecording()
{
    mScriptField->clear();
    mScriptField->setEnabled( false );

    BAListMap msg;
    msg["input"] << mStrActiveDevice.toAscii() << "start_record" << mActiveApp.toAscii();

    setActionsEnabled(false, false, false);
    if (TDriverRubyInterface::globalInstance()->executeCmd("visualization", msg, 15000, "start_record")) {
        qDebug("Recording started");
        setActionsEnabled(false, true, false);
    }
    else {
        qWarning("Recording start failed");
        QMessageBox::critical( 0, tr( "Can't start recording" ), msg.value("error").first() );
        setActionsEnabled(true, false, true);
    }
}


void TDriverRecorder::stopRecording()
{
    BAListMap msg;
    msg["input"] << mStrActiveDevice.toAscii() << "stop_record" << mActiveApp.toAscii();
    setActionsEnabled(false, false, false);

    if (TDriverRubyInterface::globalInstance()->executeCmd("visualization", msg, 20000, "stop_record")) {
        mScriptField->clear();
        mScriptField->setEnabled( true );

        QString path(outputPath + "/visualizer_rec_fragment.rb");
        QFile data( path );

        if ( data.open( QFile::ReadOnly ) ) {
            QTextStream stream( &data );
            QString line;

            do {
                line = stream.readLine();
                if ( !line.isNull() ) {
                    mScriptField->append( line );
                }
            } while( !line.isNull() );

        } else {
            QMessageBox::critical( 0, tr( "Error" ), "Could not open recorded script file.");
        }
    }
    else {
        TDriverRubyInterface::globalInstance()->requestClose();
        QMessageBox::critical( 0,
                              tr( "Can't stop recording" ),
                              tr( "Requesting tdriver_interact.rb termination after error:\n\n%1")
                              .arg(QString::fromLatin1(msg.value("error").first() )));
        qDebug("Recording stop failed, aborting anyway");
    }
    setActionsEnabled(true, false, true);
}


void TDriverRecorder::testRecording()
{
    mRecButton->setEnabled( false );
    mStopButton->setEnabled( false );
    mTestButton->setEnabled( false );

    QFile file( outputPath + "/visualizer_rec_fragment.rb" );

    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) ){
        QMessageBox::critical( 0, tr( "Error" ), "Could not store recorded script file.");

    }
    else {
        QTextDocument* doc = mScriptField->document();

        for( int i = 0 ; i < doc->lineCount(); i++ ){

            file.write( doc->findBlockByLineNumber( i ).text().toUtf8().data() );
            // file.write( doc->findBlockByLineNumber( i ).text().toAscii().data() );

            file.write( "\n" );

        }

        file.close();


        BAListMap msg;
        msg["input"] << mStrActiveDevice.toAscii() + "test_record";
        setActionsEnabled(false, false, false);
        if (TDriverRubyInterface::globalInstance()->executeCmd("visualization", msg, 15000, "test_record")) {
            QMessageBox::critical( 0, tr( "Recording test ok" ), tr("Success"));
        }
        else {
            QMessageBox::critical( 0, tr( "Can't test recording" ), msg.value("error").first() );
        }
        setActionsEnabled(true, false, true);
    }
}



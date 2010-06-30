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

#include "tdriver_recorder.h"

/*!
    \class TDriverRecorder
    \brief Uses TDRIVER to record script fragments from testable applications.
    
    TDriverRecorder uses TDRIVER framework to record scripts from testable qt
    applications. Certain mouse events are tracked and test script fragment
    is generated from them. Ouputs the fragment to an editor window.
    
 */

TDriverRecorder::TDriverRecorder( RubyThread& thread, QWidget* parent ) : QDialog( parent ), mThread( thread ) {

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

    setWindowTitle( "TDRIVER Visualizer Recorder" );

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

void TDriverRecorder::startRecording() {

    mRecButton->setEnabled( false );
    mStopButton->setEnabled( true );
    mTestButton->setEnabled( false );

    mScriptField->clear();
    mScriptField->setEnabled( false );

    QString command = mStrActiveDevice + " start_record " + mActiveApp;
    QString errorMessage;

    if( !mThread.execute_cmd( command, errorMessage ) ){

        QMessageBox::critical( 0, tr( "Error" ), errorMessage );

    }

}

void TDriverRecorder::stopRecording() {

    QProgressDialog progressDialog( this );

    progressDialog.setWindowTitle( tr( "Generating script" ) );

    progressDialog.setMinimumDuration( 0 );

    progressDialog.setRange( 0, 100 );

    progressDialog.setLabelText( tr( "Stopping recorder" ) );

    progressDialog.setValue( 1 );
    progressDialog.setValue( 15 );

    QString command = mStrActiveDevice + " stop_record " + mActiveApp;

    QString errorMessage;

    if(!mThread.execute_cmd( command, errorMessage ) ){

        QMessageBox::critical( 0, tr( "Error" ), errorMessage ); 

    } else {

        progressDialog.setLabelText( tr( "Generating script" ) );

        progressDialog.setValue( 50 );
        mScriptField->clear();
        mScriptField->setEnabled( true );

        QFile data( outputPath + "/visualizer_rec_fragment.rb" );

        if ( data.open( QFile::ReadOnly ) ) {

            QTextStream stream( &data );
            QString line;

            progressDialog.setValue( 65 );

            do {

                line = stream.readLine();

                if ( !line.isNull() ) { mScriptField->append( line ); }

            } while( !line.isNull() );

            progressDialog.setValue( 85 );

        } else {

            QMessageBox::critical( 0, tr( "Error" ), "Could not open recorded script file."); 

        }

    }

    progressDialog.setValue( 100 );

    mRecButton->setEnabled( true );
    mStopButton->setEnabled( false );
    mTestButton->setEnabled( true );

}

void TDriverRecorder::testRecording() {

    mRecButton->setEnabled( false );
    mStopButton->setEnabled( false );
    mTestButton->setEnabled( false );

    QFile file( outputPath + "/visualizer_rec_fragment.rb" );

    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) ){

        QMessageBox::critical( 0, tr( "Error" ), "Could not store recorded script file."); 

    } else {

        QTextDocument* doc = mScriptField->document();

        for( int i = 0 ; i < doc->lineCount(); i++ ){

            file.write( doc->findBlockByLineNumber( i ).text().toUtf8().data() ); 
            // file.write( doc->findBlockByLineNumber( i ).text().toAscii().data() );

            file.write( "\n" );

        }

        file.close();

        QString command = mStrActiveDevice + " test_record";
        QString errorMessage;

        if( !mThread.execute_cmd( command, errorMessage ) ){

            QMessageBox::critical( 0, tr( "Error" ), errorMessage ); 

        }

        mRecButton->setEnabled( true );
        mStopButton->setEnabled( false );
        mTestButton->setEnabled( true );

    }

}

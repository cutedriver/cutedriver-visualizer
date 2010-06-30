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

void MainWindow::showXMLDialog() {

    sourceEdit->setPlainText( xmlDocument.toString() );

    xmlView->show();
    xmlView->activateWindow();

    if ( xmlViewPos != QPoint( -1, -1 ) ) { xmlView->move( xmlViewPos ); }

    findStringComboBox->setFocus();

}

// This function just shows the xml file, that has been loaded into memory, in a new window.
void MainWindow::createXMLFileDataWindow() {

    xmlView = new QDialog( this );
    xmlView->setObjectName("xmlview");
    xmlView->resize( 800,500 );

    // reset find dialog position, stored before closing the dialog and restored when dialog opened
    xmlViewPos = QPoint(-1, -1);

    xmlView->setWindowTitle( "TDRIVER Visualizer XML" );

    QVBoxLayout *layout = new QVBoxLayout( xmlView );
    //QGridLayout *layout = new QGridLayout( xmlView );
    layout->setObjectName("xmlview");

    QGroupBox *xmlBox = new QGroupBox( xmlView );
    xmlBox->setObjectName("xmlview edit");
    xmlBox->setTitle("");

    QGridLayout* gridLayout = new QGridLayout( xmlBox );
    gridLayout->setObjectName("xmlview edit");

    sourceEdit = new QTextEdit;
    sourceEdit->setObjectName("xmlview edit");
    sourceEdit->setTextInteractionFlags( Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard );

    gridLayout->addWidget( sourceEdit );

    layout->addWidget( xmlBox );

    // group box with title
    QGroupBox *groupBox = new QGroupBox( xmlView );
    groupBox->setObjectName("xmlview search");
    groupBox->setTitle("Search for:");

    QGridLayout* layout2 = new QGridLayout( groupBox );
    layout2->setObjectName("xmlview search");

    // search text combobox 
    findStringComboBox = new QComboBox;
    findStringComboBox->setObjectName("xmlview search");
    findStringComboBox->setEditable( true );
    findStringComboBox->setDuplicatesEnabled( false );

    connect( findStringComboBox, SIGNAL( editTextChanged( const QString & ) ), this, SLOT( showXmlEditTextChanged( const QString & ) ) );

    // find button
    showXmlFindButton = new QPushButton( "&Find", this );
    showXmlFindButton->setObjectName("xmlview find");
    showXmlFindButton->setEnabled( false );
    showXmlFindButton->setDefault( true );
    showXmlFindButton->setAutoDefault( true );
    showXmlFindButton->setMaximumSize( 100, 30 );

    // close button
    showXmlCloseButton = new QPushButton( "&Close", this );
    showXmlCloseButton->setObjectName("xmlview close");
    showXmlCloseButton->setDefault( false );
    showXmlCloseButton->setAutoDefault( false );
    showXmlCloseButton->setMaximumSize( 100, 30 );

    showXmlMatchCase = new QCheckBox( "&Match case" );
    showXmlMatchCase->setObjectName("xmlview search matchcase");
    showXmlMatchCase->setCheckState( Qt::Unchecked );
    //connect( showXmlMatchCase, SIGNAL( stateChanged( int ) ), this, SLOT( changeSearchMatchCase( int ) ) );

    showXmlMatchEntireWord = new QCheckBox( "Match &entire word only" );
    showXmlMatchEntireWord->setObjectName("xmlview search entirewords");
    showXmlMatchEntireWord->setCheckState( Qt::Unchecked );
    //connect( showXmlMatchEntireWord, SIGNAL( stateChanged( int ) ), this, SLOT( changeSearchEntireWord( int ) ) );

    showXmlBackwards = new QCheckBox( "Search &backwards" );
    showXmlBackwards->setObjectName("xmlview search backwards");
    showXmlBackwards->setCheckState( Qt::Unchecked );
    //connect( showXmlBackwards, SIGNAL( stateChanged( int ) ), this, SLOT( changeSearchBackwards( int ) ) );

    showXmlWrapAround = new QCheckBox( "&Wrap around" );
    showXmlWrapAround->setObjectName("xmlview search wrap");
    showXmlWrapAround->setCheckState( Qt::Checked );
    //connect( showXmlWrapAround, SIGNAL( stateChanged( int ) ), this, SLOT( changeSearchWrapAround( int ) ) );

    //layout2->addWidget( findStringLineEdit );
    layout2->addWidget( findStringComboBox, 0, 0, 1, 4 );

    layout2->addWidget( showXmlMatchCase, 2, 0);
    layout2->addWidget( showXmlMatchEntireWord, 3, 0);
    layout2->addWidget( showXmlBackwards, 2, 1);    
    layout2->addWidget( showXmlWrapAround, 3, 1);

    layout2->addWidget( showXmlFindButton, 2, 3);
    layout2->addWidget( showXmlCloseButton, 3, 3);

    layout->addWidget( groupBox );

    connect( showXmlFindButton, SIGNAL( pressed() ), this, SLOT( findStringFromXml( ) ) );
    connect( showXmlCloseButton, SIGNAL( pressed() ), this, SLOT( closeXmlDialog() ) );

    findStringComboBox->installEventFilter( this );

}

void MainWindow::findStringFromXml() {

    //qDebug() << "findStringFromXml";

    bool found = false;
    bool result = false;

    int retry = 0;

    // do not perform search with empty string...
    if ( findStringComboBox->currentText().isEmpty() ) { return; }

    QTextCursor beginPos = sourceEdit->textCursor();

    while ( !result )  {

        QTextCursor cursorPos = sourceEdit->textCursor();

        QTextDocument::FindFlags flags;

        if ( showXmlBackwards->isChecked() )       { flags = flags | QTextDocument::FindBackward; }
        if ( showXmlMatchCase->isChecked() )       { flags = flags | QTextDocument::FindCaseSensitively; }
        if ( showXmlMatchEntireWord->isChecked() ) { flags = flags | QTextDocument::FindWholeWords; }

        found = sourceEdit->find ( findStringComboBox->currentText(), flags );

        if ( found )  {

            if ( cursorPos == sourceEdit->textCursor() && showXmlWrapAround->isChecked() ) {

                //qDebug() << "cursor didn't move, move to top";
                sourceEdit->moveCursor( showXmlBackwards->isChecked() ? QTextCursor::End : QTextCursor::Start );

            } else {
                result = true;
            }

        } else {

            // not found, wrap if checkbox enabled
            if ( retry == 0 && showXmlWrapAround->isChecked() ) {

                sourceEdit->moveCursor( showXmlBackwards->isChecked() ? QTextCursor::End : QTextCursor::Start );

                retry++;

            } else {

                // no matches found, check if text already selected? if not, show warning popup
                if ( !sourceEdit->textCursor().selectedText().contains( findStringComboBox->currentText(), showXmlMatchCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive ) ) {
            
                    sourceEdit->setTextCursor( beginPos );

                    QMessageBox::warning( 0, tr( "Find" ), " No matches found with '" + findStringComboBox->currentText() + "' " );


                }

                break;
            }
        }
    
    }

}

void MainWindow::closeXmlDialog() {

    // store dialog position
    xmlViewPos = xmlView->pos();

    xmlView->close();

}

void MainWindow::showXmlEditTextChanged( const QString & text ) {

    showXmlFindButton->setEnabled( !text.isEmpty() );
    // qDebug() << "showXmlEditTextChanged";

}




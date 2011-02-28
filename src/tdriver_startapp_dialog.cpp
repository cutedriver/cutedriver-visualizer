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


//#include <tdriver_combolineedit.h>
#include "tdriver_main_window.h"
//#include <tdriver_debug_macros.h>

#include <QGridLayout>
#include <QShortcut>
#include <QDebug>




void MainWindow::showStartAppDialog() {

//    startAppDialogSubtreeRoot = NULL;
//    if (startAppDialogSubtreeOnly->checkState() == Qt::PartiallyChecked)
//        startAppDialogSubtreeOnly->setCheckState(Qt::Checked);
    startAppDialog->show();

//    if ( startAppDialogPos != QPoint( -1, -1 ) ) { startAppDialog->move( startAppDialogPos ); }

    startAppDialogTextLineEdit->setFocus();

}

void MainWindow::closeStartAppDialog() {

    // store find dialog position
//    startAppDialogPos = startAppDialog->pos();

    startAppDialog->close();

}

void MainWindow::startApp(){

    // start new application trhough tdriver using  execute_tdriver_command
    // add app to appMenu and appBar or force a refresh (see how its done in other places
    // set new app to forground

    // Check with testability

    qDebug() << "Executing app" + startAppDialogTextLineEdit->text() ;

//    executeTDriverCommand(type, commandstring);
//    updateApplicationsList();
}

void MainWindow::startAppDialogEnableStartButton(const QString & text){
    qDebug() << "Setting Start enabled";
    if ( text.isEmpty() )
    {
        startAppDialogStartButton->setDisabled(true);
    }
    else {
        startAppDialogStartButton->setDisabled(false);
    }
}

void MainWindow::createStartAppDialog() {

//    startAppDialogSubtreeRoot = NULL;

    startAppDialog = new QDialog( this );
    startAppDialog->setObjectName( "main startapp" );
    startAppDialog->setWindowTitle( "Start new application" );
    //startAppDialog->setFixedSize( 560, 155 );

    // reset find dialog position, stored before closing the dialog and restored when dialog opened
//    startAppDialogPos = QPoint(-1, -1);

    // main layout
    QVBoxLayout *layout = new QVBoxLayout( startAppDialog );
    layout->setObjectName("main find");

    // group box with title
    QGroupBox *groupBox = new QGroupBox();
    groupBox->setObjectName("main start app group");
    groupBox->setTitle( "Start Application:" );

    // groupBox layout
    QGridLayout *groupBoxLayout = new QGridLayout( groupBox );
    groupBoxLayout->setObjectName("main start app group");

//    // search text combobox
//    startAppDialogText = new TDriverComboLineEdit();
//    startAppDialogText->setObjectName("main find text");

//    // buttons: find & close
//    startAppDialogFindButton = new QPushButton( "&Find", this );
//    startAppDialogFindButton->setObjectName("main find find");
//    startAppDialogFindButton->setEnabled( false );
//    startAppDialogFindButton->setDefault( true );
//    startAppDialogFindButton->setAutoDefault( true );

    startAppDialogTextLineEdit = new QLineEdit(this);
    // startAppDialogTextLineEdit->setValidator(new QRegExpValidator( new QRegExp("")));
    // connect to start button on validator changed and
    //

    startAppDialogWithTestability = new QCheckBox( "With -&testability argument" );
    startAppDialogWithTestability->setObjectName("main startapp withtestability");
    startAppDialogWithTestability->setCheckState( Qt::Checked );

    startAppDialogStartButton = new QPushButton( "&Start", this );
    startAppDialogStartButton->setObjectName("main startapp start");
    startAppDialogStartButton->setDisabled(true);
    startAppDialogStartButton->setDefault( false );
    startAppDialogStartButton->setAutoDefault( false );

    startAppDialogCloseButton = new QPushButton( "&Close", this );
    startAppDialogCloseButton->setObjectName("main startapp close");
    startAppDialogCloseButton->setDefault( false );
    startAppDialogCloseButton->setAutoDefault( false );



//    startAppDialogSubtreeOnly = new QCheckBox( "Current &subtree only" );
//    startAppDialogSubtreeOnly->setObjectName("main find subtree");
//    startAppDialogSubtreeOnly->setTristate(false);

    // populate widgets
//    startAppDialogText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
//    groupBoxLayout->addWidget( startAppDialogText, 0, 0, 1, -1 );

//    groupBoxLayout->addWidget( startAppDialogMatchCase, 1, 0 );
//    groupBoxLayout->addWidget( startAppDialogBackwards, 1, 1 );
//    groupBoxLayout->addWidget( startAppDialogAttributes, 1, 2 );

//    groupBoxLayout->addWidget( startAppDialogEntireWords, 2, 0 );
//    groupBoxLayout->addWidget( startAppDialogWrapAround, 2, 1 );
//    groupBoxLayout->addWidget( startAppDialogSubtreeOnly, 2, 2 );

//    groupBoxLayout->addWidget( startAppDialogFindButton, 1, 3 );


    groupBoxLayout->addWidget( startAppDialogTextLineEdit, 0, 0, 1, 3);
    groupBoxLayout->addWidget( startAppDialogWithTestability, 1, 0);
    groupBoxLayout->addWidget( startAppDialogStartButton, 1, 1);
    groupBoxLayout->addWidget( startAppDialogCloseButton, 1, 2 );

    layout->addWidget( groupBox );

//    connect( startAppDialogSubtreeOnly, SIGNAL(stateChanged(int)),
//             this, SLOT(startAppDialogSubtreeChanged(int)));
//    connect( startAppDialogText, SIGNAL( editTextChanged( const QString & ) ),
//             this, SLOT( startAppDialogTextChanged( const QString & ) ) );
//    connect( startAppDialogText, SIGNAL(triggered(QString)), this, SLOT(findNextTreeObject()));

//    connect( startAppDialogFindButton, SIGNAL( clicked() ), startAppDialogText, SLOT(externallyTriggered()));
//    connect( startAppDialogFindButton, SIGNAL( clicked() ), this, SLOT( findNextTreeObject() ) );

    connect( startAppDialogTextLineEdit, SIGNAL( textChanged(const QString &) ), this, SLOT( startAppDialogEnableStartButton(const QString &) ) );
    connect( startAppDialogStartButton, SIGNAL( clicked() ), this, SLOT( startApp() ) );
    connect( startAppDialogCloseButton, SIGNAL( clicked() ), this, SLOT( closeStartAppDialog() ) );

//    Q_ASSERT(objectTree);
//    connect (objectTree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
//             this, SLOT(sStartAppDialogHandleTreeCurrentChange(QTreeWidgetItem*)));
}


//void MainWindow::createStartAppDialogShortcuts()
//{
//    // create global shortcuts for object tree find
//    QShortcut *findSC;

//    findSC = new QShortcut(QKeySequence("Ctrl+F"), centralWidget(), 0, 0, Qt::WidgetWithChildrenShortcut);
//    connect(findSC, SIGNAL(activated()), findAction, SLOT(trigger()));

//    findSC = new QShortcut(QKeySequence("Ctrl+F"), imageViewDock, 0, 0, Qt::WidgetWithChildrenShortcut);
//    connect(findSC, SIGNAL(activated()), findAction, SLOT(trigger()));

//    findSC = new QShortcut(QKeySequence("Ctrl+F"), propertiesDock, 0, 0, Qt::WidgetWithChildrenShortcut);
//    connect(findSC, SIGNAL(activated()), findAction, SLOT(trigger()));
//}

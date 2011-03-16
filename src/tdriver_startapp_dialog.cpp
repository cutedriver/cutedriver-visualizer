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
    startAppDialog->show();
    startAppDialogTextLineEdit->setFocus();
}

void MainWindow::closeStartAppDialog() {
    startAppDialog->close();
}

void MainWindow::startApp(){
    QString app_name = startAppDialogTextLineEdit->text();
    QString app_arguments = ( startAppDialogWithTestability->isChecked() ) ? "-testability" : "";
    QString cmd = activeDevice + " start_application " + app_name + " " + app_arguments;

    //qDebug() << "Executing app" + cmd;

    if ( !executeTDriverCommand( commandStartApplication, cmd)) {
        statusbar( "Error: Failed to start application.", 1000 );
        disconnectExclusiveSUT();
    }
    else {
        startAppDialog->hide();
        statusbar( "Starting application...", 1000 );
        refreshData();
    }
    startAppDialog->close();
}

void MainWindow::startAppDialogEnableStartButton(const QString & text){

    //qDebug() << "Setting Start enabled";

    if ( text.isEmpty() )
    {
        startAppDialogStartButton->setDisabled(true);
    }
    else {
        startAppDialogStartButton->setDisabled(false);
    }
}

void MainWindow::startAppDialogReturnPress(){
    startAppDialogStartButton->click();
}

void MainWindow::createStartAppDialog() {
    startAppDialog = new QDialog( this );
    startAppDialog->setObjectName( "main startapp" );
    startAppDialog->setWindowTitle( "Start new application" );

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


    // widgets
    startAppDialogTextLineEdit = new QLineEdit(this);

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

    // layout placement

    groupBoxLayout->addWidget( startAppDialogTextLineEdit, 0, 0, 1, 3);
    groupBoxLayout->addWidget( startAppDialogWithTestability, 1, 0);
    groupBoxLayout->addWidget( startAppDialogStartButton, 1, 1);
    groupBoxLayout->addWidget( startAppDialogCloseButton, 1, 2 );

    layout->addWidget( groupBox );

    // signals to slots

    connect( startAppDialogTextLineEdit, SIGNAL( textChanged(const QString &) ), this, SLOT( startAppDialogEnableStartButton(const QString &) ) );
    connect( startAppDialogTextLineEdit, SIGNAL( returnPressed() ), this, SLOT( startAppDialogReturnPress() ) );
    connect( startAppDialogStartButton, SIGNAL( clicked() ), this, SLOT( startApp() ) );
    connect( startAppDialogCloseButton, SIGNAL( clicked() ), this, SLOT( closeStartAppDialog() ) );
}

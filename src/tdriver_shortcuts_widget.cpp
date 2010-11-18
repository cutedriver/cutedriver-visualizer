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

#include "flowlayout.h"

void MainWindow::createShortcuts() {

    //QHBoxLayout *layout = new QHBoxLayout;
    FlowLayout *layout = new FlowLayout;
    layout->setObjectName("shortcuts");
    shortcutsDock = new QDockWidget( tr( " Shortcuts" ), this );
    shortcutsDock->setObjectName("shortcuts");
    shortcutsDock->setFeatures( DOCK_FEATURES_DEFAULT );
    //shortcutsDock->setAllowedAreas( Qt::BottomDockWidgetArea );

    int i=5;
    i=6;

    horizontalBottomButtonGroupBox = new QGroupBox();
    horizontalBottomButtonGroupBox->setObjectName("shortcuts");

    // Button refresh - command

    refreshButton = new QPushButton();
    refreshButton->setObjectName("shortcuts refresh");
    refreshButton->setText( QApplication::translate( "MainWindow", "Refresh", 0, QApplication::UnicodeUTF8 ) );
    layout->addWidget( refreshButton );
    QObject::connect( refreshButton, SIGNAL( pressed() ), this, SLOT( forceRefreshData() ) );

    // Delayed Button refresh - command

    delayedRefreshButton = new QPushButton();
    delayedRefreshButton->setObjectName("shortcuts delayedrefresh");
    delayedRefreshButton->setText( QApplication::translate( "MainWindow", "Refresh in 5 s", 0, QApplication::UnicodeUTF8 ) );
    layout->addWidget( delayedRefreshButton );
    QObject::connect( delayedRefreshButton, SIGNAL( pressed() ), this, SLOT( delayedRefreshData() ) );

    // Button showXML - command

    showXMLButton = new QPushButton();
    showXMLButton->setObjectName("shortcuts showxmls");
    showXMLButton->setText( QApplication::translate( "MainWindow", "Show XML", 0, QApplication::UnicodeUTF8 ) );
    layout->addWidget( showXMLButton );
    QObject::connect( showXMLButton, SIGNAL( pressed() ), this, SLOT( showXMLDialog() ) );

    // Button load - command

    loadFileButton = new QPushButton();
    loadFileButton->setObjectName(" shortcuts loadfile");
    loadFileButton->setText( QApplication::translate( "MainWindow", "Load File", 0, QApplication::UnicodeUTF8 ) );
    layout->addWidget( loadFileButton );
    QObject::connect( loadFileButton, SIGNAL( pressed() ), this, SLOT( loadFileData() ) );

    // Button quit - command

    quitButton = new QPushButton();
    quitButton->setObjectName("shortcuts quit");
    quitButton->setText( QApplication::translate( "MainWindow", "Exit", 0, QApplication::UnicodeUTF8 ) );
    layout->addWidget( quitButton );
    QObject::connect( quitButton, SIGNAL( pressed() ), this, SLOT( close() ) );


    // Apply layout
    horizontalBottomButtonGroupBox->setLayout( layout );

    shortcutsDock->setWidget( horizontalBottomButtonGroupBox );
    addDockWidget( Qt::BottomDockWidgetArea, shortcutsDock, Qt::Vertical );

}


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


void MainWindow::createClipboardDock() {

    clipboardDock = new QDockWidget( tr(" Clipboard contents"), this );
    clipboardDock->setObjectName("clipboard");
    //clipboardDock->setAllowedAreas( Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea );

    clipboardDock->setFeatures( DOCK_FEATURES_DEFAULT );

    clipboardDock->setFloating( false );
    clipboardDock->setVisible( true );

    objectAttributeLineEdit = new QLineEdit;
    objectAttributeLineEdit->setObjectName("clipboard");

    clipboardDock->setWidget( objectAttributeLineEdit );

    addDockWidget(Qt::TopDockWidgetArea, clipboardDock, Qt::Vertical);

}

void MainWindow::updateClipboardText( QString text, bool appendText ) {

    QClipboard *clipboard = QApplication::clipboard();

    if ( appendText ) { text.prepend( clipboard->text().append(".") ); }

    clipboard->setText( text );
    objectAttributeLineEdit->setText( text );

}


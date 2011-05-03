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
#include "tdriver_image_view.h"

#include <tdriver_tabbededitor.h>
#include <tdriver_runconsole.h>
#include <tdriver_debugconsole.h>
#include <tdriver_rubyinteract.h>
#include <tdriver_editbar.h>
#include <tdriver_editor_common.h>

#include <QDockWidget>
#include <QVBoxLayout>
#include <QLabel>



void MainWindow::createEditorDocks()
{
    editorDock = new QDockWidget(tr("Code Editor"));
    editorDock->setObjectName("editor");
    runDock = new QDockWidget(tr("Script Console"));
    runDock->setObjectName("editor rundock");
    debugDock = new QDockWidget(tr("Debugger Console"));
    debugDock->setObjectName("editor debugdock");
    irDock = new QDockWidget(tr("RubyInteract Console"));
    irDock->setObjectName("editor irdock");

    tabEditor = new TDriverTabbedEditor(editorDock, this);
    runConsole = new TDriverRunConsole(true, this);
    debugConsole = new TDriverDebugConsole(this);
    irConsole = new TDriverRubyInteract(this);

    //tabEditor->newFile();

    Q_ASSERT(defaultFont);
    tabEditor->setEditorFont(*defaultFont);
    connect(this, SIGNAL(defaultFontSet(QFont)), tabEditor, SLOT(setEditorFont(QFont)));
    connect(this, SIGNAL(insertToCodeEditor(QString,bool,bool)), tabEditor, SLOT(smartInsert(QString,bool,bool)));

    // make tabEditor emit requestRunPreparations and then wait for proceedRun
    connect(this, SIGNAL(disconnectionOk(bool)), tabEditor, SLOT(proceedRun(bool)));
    connect(tabEditor, SIGNAL(requestRunPreparations(QString)), this, SLOT(disconnectExclusiveSUT()));
    tabEditor->setNeedRunPreparations(true);

    Q_ASSERT(imageWidget);
    connect(imageWidget, SIGNAL(insertToCodeEditor(QString,bool,bool)), tabEditor, SLOT(smartInsert(QString,bool,bool)));

    //createMenu();

    QVBoxLayout *editLayout = new QVBoxLayout();
    editLayout->setObjectName("editor");
    QHBoxLayout *editBarLayout = new QHBoxLayout();
    editBarLayout->addWidget(tabEditor->createEditorMenuBar());
    editBarLayout->addWidget(tabEditor->searchBar());
    editLayout->addLayout(editBarLayout);
    editLayout->addWidget(tabEditor);
    //editLayout->addWidget(tabEditor->createStatusBar());

    QWidget *editContainer = new QWidget;
    editContainer->setObjectName("editor container");
    editContainer->setLayout(editLayout);
    editorDock->setWidget(editContainer);

    //editorDock->setWidget(tabEditor);
    runDock->setWidget(runConsole);
    debugDock->setWidget(debugConsole);
    irDock->setWidget(irConsole);

    //irDock->setFeatures(QDockWidget::DockWidgetFloatable|QDockWidget::DockWidgetMovable);
    tabEditor->connectConsoles(runConsole, runDock, debugConsole, debugDock, irConsole, irDock);
}

void MainWindow::setEditorDocksDefaultLayout()
{
    editorDock->setFloating(false);
    runDock->setFloating(false);
    debugDock->setFloating(false);
    irDock->setFloating(false);

    addDockWidget(Qt::BottomDockWidgetArea, editorDock, Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea, runDock, Qt::Vertical);
    addDockWidget(Qt::BottomDockWidgetArea, debugDock, Qt::Vertical);
    addDockWidget(Qt::BottomDockWidgetArea, irDock, Qt::Vertical);

    editorDock->setVisible(false);
    debugDock->setVisible(false);
    runDock->setVisible(false);
    irDock->setVisible(false);
}

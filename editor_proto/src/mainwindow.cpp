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


#include "mainwindow.h"
#include "tdriver_tabbededitor.h"
#include "tdriver_runconsole.h"
#include "tdriver_debugconsole.h"
#include "tdriver_rubyinteract.h"
#include <tdriver_editbar.h>
#include <tdriver_editor_common.h>
#include <tdriver_rubyinterface.h>

#include <QApplication>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QDockWidget>
#include <QSettings>
#include <QCloseEvent>
#include <QStringList>

#include <tdriver_debug_macros.h>

MainWindow::MainWindow(QStringList filelist, QWidget *parent) :
        QMainWindow(parent)
{
    TDriverRubyInterface::startGlobalInstance();

    tabs = new TDriverTabbedEditor(this, this);
    runConsole = new TDriverRunConsole(true, this);
    debugConsole = new TDriverDebugConsole(this);
    irConsole = new TDriverRubyInteract(this);

    tabs->setObjectName("editor");
    runConsole->setObjectName("run");
    debugConsole->setObjectName("debug");

    createMenu();

    setCentralWidget(tabs);

    QDockWidget *runDock = new QDockWidget("Ruby Script Output");
    runDock->setObjectName("run");
    addDockWidget(Qt::RightDockWidgetArea, runDock);
    runDock->setWidget(runConsole);
    runDock->setVisible(false);

    QDockWidget *debugDock = new QDockWidget("Debugger Console");
    debugDock->setObjectName("debug");
    addDockWidget(Qt::RightDockWidgetArea, debugDock);
    debugDock->setWidget(debugConsole);
    debugDock->setVisible(false);

    QDockWidget *irDock = new QDockWidget("Ruby interaction Console");
    irDock->setObjectName("iruby");
    addDockWidget(Qt::BottomDockWidgetArea, irDock);
    irDock->setWidget(irConsole);
    irDock->setVisible(false);

    QDockWidget *searchBarDock = new QDockWidget("Search Bar");
    searchBarDock->setObjectName("searchbar");
    addDockWidget(Qt::TopDockWidgetArea, searchBarDock);
    searchBarDock->setWidget(tabs->searchBar());
    searchBarDock->setVisible(true);
    searchBarDock->setFeatures(QDockWidget::NoDockWidgetFeatures);

    tabs->connectConsoles(runConsole, runDock, debugConsole, debugDock, irConsole, irDock);

    QSettings settings;
    restoreGeometry(settings.value("editor/geometry").toByteArray());
    restoreState(settings.value("editor/windowstate").toByteArray());

    // note: code below must be after connectConsoles
    if (filelist.isEmpty()) {
        tabs->newFile();
    }
    else {
        foreach (QString file, filelist) {
            file = MEC::fileWithPath(file);
            if (!tabs->loadFile(file)) tabs->newFile(file);
        }
    }
}


void MainWindow::createMenu()
{
    QAction *exitAction = new QAction(tr("E&xit"), this);
    exitAction->setObjectName("exit");
    QAction *aboutAct = new QAction(tr("About"), this);
    aboutAct->setObjectName("about");
    QAction *aboutQtAct = new QAction(tr("About Qt"), this);
    aboutQtAct->setObjectName("aboutqt");

    connect(exitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    menuBar()->actions().last()->setObjectName("file");
    fileMenu->setObjectName("file");
    fileMenu->addActions(tabs->fileActions());
    fileMenu->addSeparator();
    fileMenu->addActions(tabs->recentFileActions());
    tabs->updateRecentFileActions();
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    menuBar()->actions().last()->setObjectName("edit");
    editMenu->setObjectName("edit");
    editMenu->addActions(tabs->editActions());
    editMenu->addSeparator();
    editMenu->addActions(tabs->codeActions());

    QMenu *optMenu = menuBar()->addMenu(tr("&Toggles"));
    menuBar()->actions().last()->setObjectName("opt");
    optMenu->setObjectName("opt");

    optMenu->addActions(tabs->optionActions());

    QMenu *runMenu = menuBar()->addMenu(tr("&Run"));
    menuBar()->actions().last()->setObjectName("run");
    runMenu->setObjectName("run");
    runMenu->addActions(tabs->runActions());

    QMenu* aboutMenu = menuBar()->addMenu(tr("&About"));
    menuBar()->actions().last()->setObjectName("about");
    aboutMenu->setObjectName("about");
    aboutMenu->addAction(aboutAct);
    aboutMenu->addAction(aboutQtAct);
}


void MainWindow::closeEvent(QCloseEvent *ev)
{
    if (!tabs->mainCloseEvent(ev)) {
        //qDebug() << FFL << "tabs rejected close, ignoring event";
        ev->ignore();
    }
    else {
        //qDebug() << FFL << "doing saveGeometry and saveState";
        QSettings settings;
        settings.setValue("editor/geometry", saveGeometry());
        settings.setValue("editor/windowstate", saveState());
        ev->accept();
    }
}


void MainWindow::about()
{
    QMessageBox::about(this, tr("About"), tr("TDriver Visualizer") + "\n" + tr("Code Editor, standalone version."));
}

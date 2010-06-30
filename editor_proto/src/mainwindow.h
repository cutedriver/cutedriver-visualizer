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
 
 
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QSettings;
class QCloseEvent;
class QStringList;

class TDriverTabbedEditor;
class TDriverDebugConsole;
class TDriverRunConsole;
class TDriverRubyInteract;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QStringList filelist = QStringList(), QWidget *parent = 0);

protected:
    void createMenu();
    virtual void closeEvent(QCloseEvent *);

private slots:
    void about();

private:
    TDriverTabbedEditor *tabs;
    TDriverRunConsole *runConsole;
    TDriverDebugConsole *debugConsole;
    TDriverRubyInteract *irConsole;
};

#endif // MAINWINDOW_H

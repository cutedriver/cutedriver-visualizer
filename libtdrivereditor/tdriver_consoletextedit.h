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


#ifndef TDRIVER_CONSOLETEXTEDIT_H
#define TDRIVER_CONSOLETEXTEDIT_H

#include "libtdrivereditor_global.h"

#include <QPlainTextEdit>
#include <QTextCharFormat>

class QIODevice;
class QTextCursor;
class QColor;
class QToolBar;
class TDriverComboLineEdit;

class LIBTDRIVEREDITORSHARED_EXPORT TDriverConsoleTextEdit : public QPlainTextEdit
{
Q_OBJECT
public:
    explicit TDriverConsoleTextEdit(QWidget *parent = 0);
    const QTextCursor &appendCursor() { return *appendCur; }
    TDriverComboLineEdit *commandLine() { return cmdLine; }
    void keyPressEvent(QKeyEvent *);
    //void keyReleaseEvent(QKeyEvent *);

    // make these public for easy manipulation, as this class is meant to be embedded into another
    QTextCharFormat commandFormat;
    QTextCharFormat outputFormat;
    QTextCharFormat notifyFormat;

public slots:
    void disconnectCommandLine();
    void configureCommandLine(TDriverComboLineEdit *);
    bool sendAndAppendCommand(QString text);
    bool sendAndAppendChars(QString text,  const QTextCharFormat &format);
    void setIODevice(QIODevice *);
    void setLocalEcho(bool);
    void setQuiet(bool);

    void copyAppendCursor(TDriverConsoleTextEdit *source);

    void appendLine(const QString &text, const QTextCharFormat &format);
    void appendText(const QString &text, const QTextCharFormat &format);
    void centerAppendCursor();

protected:
    virtual void insertFromMimeData ( const QMimeData * source );
    TDriverComboLineEdit *cmdLine;

private:
    QIODevice *io;
    bool localEcho;
    bool quiet;
    QTextCursor *appendCur;
    TDriverConsoleTextEdit *appendCurSource;
};

#endif // TDRIVER_CONSOLETEXTEDIT_H

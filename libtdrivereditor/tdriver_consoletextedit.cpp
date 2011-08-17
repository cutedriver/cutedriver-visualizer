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


#include "tdriver_consoletextedit.h"
#include "tdriver_combolineedit.h"
#include "tdriver_editor_common.h"

#include <QApplication>
#include <QString>
#include <QIODevice>
#include <QByteArray>
#include <QToolBar>
#include <QTextCharFormat>

#include <tdriver_debug_macros.h>


TDriverConsoleTextEdit::TDriverConsoleTextEdit(QWidget *parent) :
        QPlainTextEdit(parent),
        cmdLine(NULL),
        io(NULL),
#ifdef Q_WS_WIN
        localEcho(false),
#else
        localEcho(true),
#endif
        quiet(false),
        appendCur(new QTextCursor(document())),
        appendCurSource(this)
{
    //setWordWrapMode(QTextOption::NoWrap);

    // note: command line is not displayed by default, it's responsibility of derived class to display it
    configureCommandLine(new TDriverComboLineEdit(this));
    cmdLine->setObjectName("console command line");
    cmdLine->hide();

    commandFormat.setFontItalic(true);
    commandFormat.setForeground(QBrush(Qt::darkGreen));

    outputFormat.setForeground(QBrush(Qt::darkGray));

    notifyFormat.setFontWeight(QFont::Bold);
}


void TDriverConsoleTextEdit::disconnectCommandLine()
{
    if (cmdLine)
        disconnect(cmdLine, SIGNAL(triggered(QString)), this, SLOT(sendAndAppendCommand(QString)));
}

void TDriverConsoleTextEdit::configureCommandLine(TDriverComboLineEdit *cbl)
{
    if (cmdLine != cbl) {
        if (cmdLine && cmdLine->parent() == this) {
            delete cmdLine;
        }
        cmdLine = cbl;
    }
    else if (cmdLine && cmdLine == cbl) {
        // avoid accidental duplicate connection
        disconnectCommandLine();
    }

    if (cmdLine) {
        connect(cmdLine, SIGNAL(triggered(QString)), this, SLOT(sendAndAppendCommand(QString)));
    }
}


void TDriverConsoleTextEdit::setLocalEcho(bool state)
{
    localEcho = state;
}


void TDriverConsoleTextEdit::setQuiet(bool state)
{
    quiet = state;
}



void TDriverConsoleTextEdit::copyAppendCursor(TDriverConsoleTextEdit *source)
{
    if (appendCur) delete appendCur;
    if (source) {
        appendCur = new QTextCursor(*source->appendCur);
        appendCurSource = source;
    }
    else {
        appendCur = new QTextCursor(document());
        appendCurSource = this;
    }
}


bool TDriverConsoleTextEdit::sendAndAppendCommand(QString text)
{
    //qDebug() << FCFL << "text" << text << "ro" << isReadOnly() << "io" << io << "lecho" << localEcho << "q" << quiet;

    if (!text.endsWith('\n')) text.append('\n');
    return sendAndAppendChars(text, commandFormat);
}

bool TDriverConsoleTextEdit::sendAndAppendChars(QString text,  const QTextCharFormat &format)
{
    //qDebug() << FCFL << "text" << text << "ro" << isReadOnly() << "io" << io << "lecho" << localEcho << "q" << quiet;
    if (text.isEmpty() || !io) return false;

    MEC::replaceUnicodeSeparators(text);

    if (!quiet) {
        moveCursor(QTextCursor::End);
        centerCursor();
        if (localEcho) {
            appendCur->setCharFormat(format);
            appendCur->insertText(text);
        }
    }
    //emit keyText(text);

    //qDebug() << FCFL << "WRITE io" << text.toLatin1();
    io->write(text.toLatin1()); // TODO: check if all wasn't written and do something about it
    return true;
}


void TDriverConsoleTextEdit::keyPressEvent(QKeyEvent *event)
{
    QString text(event->text());
    static QSet<int> alwaysToTextEdit;
    if (alwaysToTextEdit.isEmpty()) {
        alwaysToTextEdit
                << Qt::Key_Up << Qt::Key_Down
                << Qt::Key_Left << Qt::Key_Right
                << Qt::Key_PageUp << Qt::Key_PageDown;
    }

    if (cmdLine && cmdLine->isVisible() && !alwaysToTextEdit.contains(event->key())) {
        QCoreApplication::sendEvent(cmdLine, event);
    }
    else {
        if (text.isEmpty()) {
            QPlainTextEdit::keyPressEvent(event);
        }
        else if (!isReadOnly()) {
            if (text == "\r ") text = "\n";
            sendAndAppendChars(text, outputFormat);
        }
        else {
            qDebug() << FCFL << "readonly, keypress ignored";
        }
    }
}


void TDriverConsoleTextEdit::setIODevice(QIODevice *qio)
{
    if (io && io!=qio && io->parent() == this) delete io;
    io = qio;
}


void TDriverConsoleTextEdit::appendLine(const QString &text, const QTextCharFormat &format)
{
    if (quiet) return;

    QString postfix( text.endsWith('\n') ? "" : "\n");
    appendText(text+postfix, format);
}


void TDriverConsoleTextEdit::appendText(const QString &text, const QTextCharFormat &format)
{
    if (quiet) return;

    appendCur->setCharFormat(format);
    appendCur->insertText(text);
    centerAppendCursor();
}

void TDriverConsoleTextEdit::centerAppendCursor()
{
    //QTextCursor tmp = appendCurSource->textCursor();
    appendCurSource->setTextCursor(*appendCur);
    appendCurSource->ensureCursorVisible();
    //appendCurSource->setTextCursor(tmp);
}


void TDriverConsoleTextEdit::insertFromMimeData ( const QMimeData * source )
{
    sendAndAppendChars(source->text(), commandFormat);
}

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


#ifndef TDRIVER_SEARCHWIDGET_H
#define TDRIVER_SEARCHWIDGET_H

#include <QToolBar>
#include <QTextDocument>

class TDriverComboLineEdit;
class QAction;

class TDriverEditBar : public QToolBar
{
    Q_OBJECT
    Q_PROPERTY(QString sutVariable READ sutVariable)
    Q_PROPERTY(QString appVariable READ appVariable)

public:
    explicit TDriverEditBar(QWidget *parent = 0);

    QString sutVariable() const;
    QString appVariable() const;

    TDriverComboLineEdit *findTextField() { return findField; }
    TDriverComboLineEdit *replaceTextField() { return replaceField; }

signals:
    void requestFind(QString findText, QTextDocument::FindFlags options);
    void requestFindIncremental(QString findText, QTextDocument::FindFlags options);
    void requestReplaceFind(QString findText, QString replaceText, QTextDocument::FindFlags options);
    void requestReplaceAll(QString findText, QString replaceText, QTextDocument::FindFlags options);
    void requestUnfocus();

public slots:
    void findNext();
    void findIncremental();
    void findPrev();
    void replaceFindNext();
    void replaceAll();

private:
    TDriverComboLineEdit *findField;
    QAction *findPrevAct;
    QAction *findNextAct;
    QAction *toggleCaseAct;

    TDriverComboLineEdit *replaceField;
    QAction *replaceFindNextAct;
    QAction *replaceAllAct;

    TDriverComboLineEdit *sutField;
    TDriverComboLineEdit *appField;
};

#endif // TDRIVER_SEARCHWIDGET_H

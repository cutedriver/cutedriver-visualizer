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


#include "tdriver_editbar.h"
#include "tdriver_combolineedit.h"

#include <QAction>
#include <QTextDocument>
#include <QLabel>

TDriverEditBar::TDriverEditBar(QWidget *parent) :
        QToolBar(parent),
        findField(new TDriverComboLineEdit(this)),
        findPrevAct(new QAction(tr("<"), this)),
        findNextAct(new QAction(tr(">"), this)),
        toggleCaseAct(new QAction(tr("aA"), this)),
        replaceField(new TDriverComboLineEdit(this)),
        replaceFindNextAct(new QAction(tr("!>"), this)),
        replaceAllAct(new QAction(tr("!all"), this)),
        sutField(new TDriverComboLineEdit(this)),
        appField(new TDriverComboLineEdit(this))
{
    addWidget(new QLabel(tr("Find:")));

    findField->setObjectName("search text");
    addWidget(findField);

    findPrevAct->setObjectName("search prev");
    addAction(findPrevAct);
    connect(findPrevAct, SIGNAL(triggered()), this, SLOT(findPrev()));
    connect(findPrevAct, SIGNAL(triggered()), findField, SLOT(externallyTriggered())); // add text to combobox history list

    findNextAct->setObjectName("search next");
    addAction(findNextAct);
    connect(findNextAct, SIGNAL(triggered()), this, SLOT(findNext()));
    connect(findNextAct, SIGNAL(triggered()), findField, SLOT(externallyTriggered())); // add text to combobox history list
    connect(findField, SIGNAL(triggered(QString)), this, SLOT(findNext())); // do action with enter in combobox

    connect(findField, SIGNAL(escapePressed()), this, SIGNAL(requestUnfocus()));
    connect(replaceField, SIGNAL(escapePressed()), this, SIGNAL(requestUnfocus()));
    connect(sutField, SIGNAL(escapePressed()), this, SIGNAL(requestUnfocus()));
    connect(appField, SIGNAL(escapePressed()), this, SIGNAL(requestUnfocus()));

    toggleCaseAct->setObjectName("case toggle");
    toggleCaseAct->setCheckable(true);
    toggleCaseAct->setChecked(false);
    addAction(toggleCaseAct);

    addSeparator();
    addWidget(new QLabel(tr("Replace with:")));

    replaceField->setObjectName("replace text");
    addWidget(replaceField);

    replaceFindNextAct->setObjectName("replace next");
    connect(replaceFindNextAct, SIGNAL(triggered()), this, SLOT(replaceFindNext()));
    connect(replaceFindNextAct, SIGNAL(triggered()), replaceField, SLOT(externallyTriggered())); // add text to combobox history list
    connect(replaceField, SIGNAL(triggered(QString)), this, SLOT(replaceFindNext())); // do action with enter in combobox
    addAction(replaceFindNextAct);

    replaceAllAct->setObjectName("replace all");
    connect(replaceAllAct, SIGNAL(triggered()), this, SLOT(replaceAll()));
    connect(replaceAllAct, SIGNAL(triggered()), replaceField, SLOT(externallyTriggered())); // add text to combobox history list
    addAction(replaceAllAct);
    addSeparator();

    addWidget(new QLabel(tr("sut:")));
    sutField->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sutField->setMinimumContentsLength(5);
    sutField->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    sutField->addItem("@sut");
    addWidget(sutField);

    addWidget(new QLabel(tr("app:")));
    appField->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    appField->setMinimumContentsLength(5);
    appField->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    appField->addItem("@app");
    addWidget(appField);
}


QString TDriverEditBar::sutVariable() const
{
    if (sutField) return sutField->currentText();
    else return QString();
}


QString TDriverEditBar::appVariable() const
{
    if (appField) return appField->currentText();
    else return QString();
}



void TDriverEditBar::findNext()
{
    QTextDocument::FindFlags options = 0;
    if (toggleCaseAct->isChecked()) options |= QTextDocument::FindCaseSensitively;
    emit requestFind(findField->currentText(), options);
}


void TDriverEditBar::findPrev()
{
    QTextDocument::FindFlags options = QTextDocument::FindBackward;
    if (toggleCaseAct->isChecked()) options |= QTextDocument::FindCaseSensitively;
    emit requestFind(findField->currentText(), options);
}


void TDriverEditBar::replaceFindNext()
{
    QTextDocument::FindFlags options = 0;
    if (toggleCaseAct->isChecked()) options |= QTextDocument::FindCaseSensitively;
    emit requestReplaceFind(findField->currentText(), replaceField->currentText(), options);
}


void TDriverEditBar::replaceAll()
{
    QTextDocument::FindFlags options = 0;
    if (toggleCaseAct->isChecked()) options |= QTextDocument::FindCaseSensitively;
    emit requestReplaceAll(findField->currentText(), replaceField->currentText(), options);
}

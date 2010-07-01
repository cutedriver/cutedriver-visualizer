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


#include "tdriver_combolineedit.h"

#include <QKeyEvent>
#include <QLineEdit>

TDriverComboLineEdit::TDriverComboLineEdit(QWidget *parent) :
    QComboBox(parent),
    clearOnTriggerPriv(false)
{
    setEditable(true);
    setInsertPolicy(QComboBox::InsertAtTop);
    setDuplicatesEnabled( false );
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    connect(this, SIGNAL(triggered(QString)), lineEdit(), SLOT(selectAll()));
}


void TDriverComboLineEdit::trigger(){
    externallyTriggered();
    emit triggered(currentText());
}


void TDriverComboLineEdit::externallyTriggered()
{
    if (insertPolicy() != QComboBox::NoInsert) {

        QString text(currentText());

        // emulate duplicatesEnabled property
        if (!duplicatesEnabled()) {
            for(int ii=count()-1; ii >= 0; --ii) {
                if (itemText(ii) == text) {
                    removeItem(ii);
                }
            }
        }

        // emulate insertPolicy property
        switch (insertPolicy()) {

        case QComboBox::InsertAtTop:
            insertItem(0, text);
            break;

        case QComboBox::InsertAtBottom:
            addItem(text);
            break;

        default:
            qWarning("%s:%s: unsupported insertPolicy", __FILE__, __FUNCTION__);
            addItem(text);
        }
    }
    lineEdit()->selectAll();
}


void TDriverComboLineEdit::setClearOnTrigger(bool clear)
{
    clearOnTriggerPriv = clear;
}


void TDriverComboLineEdit::keyPressEvent(QKeyEvent *event)
{
    QComboBox::keyPressEvent(event);

    switch(event->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        emit triggered(currentText());
        if (clearOnTriggerPriv) {
            clearEditText();
        }
    }
}

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

TDriverComboLineEdit::TDriverComboLineEdit(QWidget *parent) :
    QComboBox(parent),
    clearOnTriggerPriv(false)
{
    setEditable(true);
    setInsertPolicy(QComboBox::InsertAtTop);
    setDuplicatesEnabled( false );
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
}

void TDriverComboLineEdit::setClearOnTrigger(bool clear)
{
    clearOnTriggerPriv = clear;
}

void    TDriverComboLineEdit::keyPressEvent(QKeyEvent *event)
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

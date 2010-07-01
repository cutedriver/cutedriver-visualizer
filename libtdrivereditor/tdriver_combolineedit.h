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


#ifndef TDRIVER_COMBOLINEEDIT_H
#define TDRIVER_COMBOLINEEDIT_H

#include <QComboBox>
class QKeyEvent;

class TDriverComboLineEdit : public QComboBox
{
Q_OBJECT
Q_PROPERTY(bool clearOnTrigger READ clearOnTrigger WRITE setClearOnTrigger)

public:
    explicit TDriverComboLineEdit(QWidget *parent = 0);
    bool clearOnTrigger() { return clearOnTriggerPriv; }

signals:
    void triggered(QString text);

public slots:
    void trigger();
    void externallyTriggered();
    void setClearOnTrigger(bool clear);

protected:
    virtual void    keyPressEvent(QKeyEvent *);

private:
    bool clearOnTriggerPriv;
};

#endif // TDRIVER_COMBOLINEEDIT_H

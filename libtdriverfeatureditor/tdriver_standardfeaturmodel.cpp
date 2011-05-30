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

#include "tdriver_standardfeaturmodel.h"

#include <QModelIndex>

TDriverStandardFeaturModel::TDriverStandardFeaturModel(QObject *parent) :
    QStandardItemModel(0, 1, parent)
{
}

bool TDriverStandardFeaturModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    bool ret = QStandardItemModel::setData(index, value, role);

    QStandardItem *item = itemFromIndex(index);
    if (item) {
        Qt::ItemFlags flags = item->flags();
        flags &= ~Qt::ItemIsEditable;
        item->setFlags(flags);
    }

    return ret;
}

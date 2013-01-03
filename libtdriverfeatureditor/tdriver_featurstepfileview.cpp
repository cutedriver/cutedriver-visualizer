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

#include "tdriver_featurstepfileview.h"

#include <tdriver_debug_macros.h>

#include <QtGui>
#include <QAbstractItemView>

TDriverFeaturStepFileView::TDriverFeaturStepFileView(QWidget *parent) :
    TDriverFeaturAbstractView(tr("Step Definition Files"), parent)
  , __overridePath(false)
{
    // with FileSectionScan, pattern means patter end
    setScanType(DirScan);
    setScanPattern("*.rb");
    //_listView->setItemDelegate(_styleDelegate);
}


void TDriverFeaturStepFileView::resetPath(const QString &path)
{
    if (__overridePath) {
        qDebug() << FCFL << "overridePath set, ignoring" << path;
        return;
    }

    qDebug() << FCFL << path;

    QFileInfo info(path);

    if (!info.isDir()) {
        qDebug() << FCFL << "got file, switching to its dir";
        info.setFile(info.canonicalPath());
    }

    TDriverFeaturAbstractView::resetPath(info.canonicalFilePath());
}

int TDriverFeaturStepFileView::doDirScan()
{
    // update file list,
    // then make sure display is updated,
    // and schedule file content scan to be called from event loop

    int ret = TDriverFeaturAbstractView::doDirScan();
    update();
    QMetaObject::invokeMethod(this, "readFoundFiles", Qt::QueuedConnection);

    return ret;
}

void TDriverFeaturStepFileView::readFoundFiles()
{
    int rows = model()->rowCount();

    for (int row=0; row < rows; ++row) {
        QModelIndex index(model()->index(row, 0));
        QString path = model()->data(index, ActualPathRole).toString();
        QFile file(path);
        if (file.open(QFile::ReadOnly)) {
            //qDebug() << FCFL << path << "OPEN";
            model()->setData(index, file.readAll(), Qt::ToolTipRole/*FileContentRole*/);
            file.close();
            //model()->setData(index, QBrush(Qt::green), Qt::ForegroundRole);
        }
        else {
            qDebug() << FCFL << path << "ERROR" << file.errorString();
            view()->setItemDelegateForRow(row, (QAbstractItemDelegate*)_styleDelegate);
            model()->setData(index, QBrush(Qt::red), Qt::ForegroundRole);
        }
    }
}

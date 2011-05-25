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


TDriverFeaturStepFileView::TDriverFeaturStepFileView(QWidget *parent) :
    TDriverFeaturAbstractView(tr("Step Definition Files"), parent)
  , __overridePath(false)
{
    // with FileSectionScan, pattern means patter end
    setScanType(DirScan);
    setScanPattern("*.rb");
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

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


#include "tdriver_util.h"

#include <QFile>
#include <QCoreApplication>
#include <cstdlib>

TDriverUtil::TDriverUtil(QObject *parent) :
        QObject(parent)
{
}


QString TDriverUtil::tdriverHelperFilePath(const QString &filename, const QString &overrideEnvVar)
{
    QString fullpath;

    if (!overrideEnvVar.isEmpty()) {
        fullpath = getenv(overrideEnvVar.toLocal8Bit());
    }

    if (fullpath.isEmpty()) {
#ifdef Q_WS_WIN
        fullpath = "C:/tdriver/visualizer/" + filename;
#else // defined(Q_WS_X11) || defined(Q_WS_MAC)
        fullpath = "/etc/tdriver/visualizer/" + filename;
#endif
    }

    return fullpath;
}

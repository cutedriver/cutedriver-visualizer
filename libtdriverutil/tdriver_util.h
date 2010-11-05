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


#ifndef TDRIVER_UTIL_H
#define TDRIVER_UTIL_H

#include "libtdriverutil_global.h"

#include <QObject>
#include <QString>

class LIBTDRIVERUTILSHARED_EXPORT  TDriverUtil : public QObject
{
Q_OBJECT
public:
    explicit TDriverUtil(QObject *parent = 0);

    static QString helpUrlString(const char *file);
    static QString tdriverHelperFilePath(const QString &filename, const QString &overrideEnvVar=QString());
    static QString smartJoin(const QString &str1, QChar sep, const QString &str2 = QString());

signals:

public slots:

};


#endif // TDRIVER_UTIL_H

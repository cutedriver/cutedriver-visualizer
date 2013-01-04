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
#include <QStringList>

#include <cstdlib>

const char TDriverUtil::visualizationId[] = "visualization";
const char TDriverUtil::interactionId[] = "interaction";

TDriverUtil::TDriverUtil(QObject *parent) :
    QObject(parent)
{
}


QString TDriverUtil::helpUrlString(const char *file)
{
#ifdef Q_OS_WIN
        return QString("C:/tdriver/visualizer/help/")+file;
#else // defined(Q_WS_X11) || defined(Q_OS_MAC)
        return QString ("/opt/tdriver_visualizer/help/")+file;
#endif
}


QString TDriverUtil::tdriverHelperFilePath(const QString &filename, const QString &overrideEnvVar)
{
    QString fullpath;

    if (!overrideEnvVar.isEmpty()) {
        fullpath = getenv(overrideEnvVar.toLocal8Bit());
    }

    if (fullpath.isEmpty()) {
#ifdef Q_OS_WIN
        fullpath = "C:/tdriver/visualizer/" + filename;
#else // defined(Q_WS_X11) || defined(Q_OS_MAC)
        fullpath = "/etc/tdriver/visualizer/" + filename;
#endif
    }

    return fullpath;
}


QString TDriverUtil::smartJoin(const QString &str1, QChar sep, const QString &str2)
{
    int ii;

    for(ii = str1.size(); ii > 0 && str1.at(ii-1)==sep; --ii) {}

    QString result;
    result.reserve(ii + 1 + str2.size() + 1);
    result = str1.left(ii);

    if (!str2.isEmpty()) {
        result = str1.left(ii);
        result += sep;

        for(ii = 0; ii < str2.size() && str2.at(ii)==sep; ++ii) {}
        result += str2.mid(ii);
    }

    return result;
}

int TDriverUtil::quotedToInt(QString str)
{
    // this is very liberal, allowing any number of single or double quotes at start or end of string
    str.replace("'", " ");
    str.replace("\"", " ");
    return str.trimmed().toInt();
}


BAList TDriverUtil::toBAList(const QStringList &list)
{
    BAList ret;
    ret.reserve(list.size());
    for(int ii=0; ii < list.size(); ++ii) {
        ret << list.at(ii).toLatin1();
    }
    return ret;
}


QString TDriverUtil::rubySingleQuote(const QString &str)
{
    QString result(str);
    result.replace("\\", "\\\\");
    result.replace("'", "\\'");
    return "'" + result + "'";
}

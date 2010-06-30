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


#ifndef TDRIVER_EDITOR_COMMON_H
#define TDRIVER_EDITOR_COMMON_H

// Debug-related stuff

#include <QDebug>
#define FFL __FILE__ << ':' << __FUNCTION__ << ':' << __LINE__ << ": "
#define FCFL __FILE__ << QString(metaObject()->className())+"::"+__FUNCTION__ << ':' << __LINE__ << ": "

// this will make rdebug control console visible and non-quiet
#define DEBUG_RDEBUG_CONTROL 0

#include <QString>

class QStandardItem;
class QStandardItemModel;
class QFile;
class QStringList;
class QByteArray;
class QRegExp;
class QSettings;
class QTextCursor;

template <class Key, class T> class QMap;
template <class T> class QList;

namespace MEC {

    struct Breakpoint {
        int num;
        bool enabled;
        QString file;
        int line;
    };

    QString dumpBreakpoint(const struct Breakpoint *bpp);
    QString dumpBreakpointList(const QList<struct Breakpoint> &bplist, const QString prefix=QString(), const QString delim="\n", const QString suffix=QString());

    extern const QRegExp * const SpaceRegExp;

    extern QSettings *settings;

    // DefaultUserRole is just as a placeholder
    // CountRole is an integer (reference) counter
    enum { DefaultUserRole = Qt::UserRole+1, CountRole };

    // note: InvalidDefinitionFile must be last entry, and integer values must start from 0 with no gaps (may be used for array indexing).
    enum DefinitionFileType { PlainTextFile, SimpleOneLineDefinitionList, SimpleMultiLineDefinitionList, InvalidDefinitionFile };


    // these dump functions contain TDriverCompleter specific printout
    void dumpStdItem(int indent, const QStandardItem *item);
    void dumpStdModel(const QStandardItemModel *model);

    DefinitionFileType getDefinitionFile(const QString &fileName, QStringList &contents, QMap<QString, QString> *defsPtr = NULL);
    DefinitionFileType getDefinitionFile(QFile &file, QStringList &contents, QMap<QString, QString> *defsPtr = NULL);
    DefinitionFileType parseDefinitionFileType(const QByteArray &line1, QRegExp &separator, QMap<QString,QString> *defsPtr = NULL);

    int modifySelectionEnd(QTextCursor &cursor, const QString &text, int startPos=-1);

    QString absoluteFilePath(const QString &fileName);
    QString pathReplaced(const QString &fileName, const QString &newPath);

    QString textShortened(const QString &text, int left, int right);

    QString &replaceUnicodeSeparators(QString &str);

    bool isAfterEscape(QTextCursor cur);
    bool autoSelectExpression(QTextCursor &cur, const QString &lang = QString());
    bool autoSelectQuoted(QTextCursor &cur);

    char getPair(char ch);
}


#endif // TDRIVER_EDITOR_COMMON_H

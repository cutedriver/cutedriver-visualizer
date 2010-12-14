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
        Breakpoint() : num(0), enabled(false), line(0) {}

        Breakpoint(int indexNum, bool isEnabled, const QString &fileName, int lineNum) :
            num(indexNum), enabled(isEnabled), file(fileName), line(lineNum) {}

        int num; // breakpoint id in debugger
        bool enabled; // wether breakpoint is enabled or disabled
        QString file; // absolute file name of file breakpoint is in
        int line; // line number of breakpoint, starting from 1
    };

    QString dumpBreakpoint(const struct Breakpoint *bpp);
    QString dumpBreakpointList(const QList<struct Breakpoint> &bplist, const QString prefix=QString(), const QString delim="\n", const QString suffix=QString());

    extern QSettings *settings;

    enum DefinitionFileType { PlainTextFile, SimpleOneLineDefinitionList, SimpleMultiLineDefinitionList, InvalidDefinitionFile };
    // note: InvalidDefinitionFile must be last entry, and integer values must start from 0 with no gaps (may be used for array indexing).

    // these dump functions contain TDriverCompleter specific printout
    void dumpStdItem(int indent, const QStandardItem *item);
    void dumpStdModel(const QStandardItemModel *model);

    DefinitionFileType getDefinitionFile(const QString &fileName, QStringList &contents, QMap<QString, QString> *defsPtr = NULL);
    // gets contents of given definition file
    // returns: type of definition file, and contents and header defintions in reference arguments

    DefinitionFileType getDefinitionFile(QFile &file, QStringList &contents, QMap<QString, QString> *defsPtr = NULL);
    // gets contents of already opened definition file, returns it's type
    // returns: type of definition file, and contents and header defintions in reference arguments

    DefinitionFileType parseDefinitionFileType(const QByteArray &line1, QRegExp &separator, QMap<QString,QString> *defsPtr = NULL);
    // parses definition file header line
    // returns: type of definition file, and in reference arguments the regexp for splitting the contents, and header defintions

    int modifySelectionEnd(QTextCursor &cursor, const QString &text, int startPos=-1);
    // replaces text in selection after startPos with text, while keeping selection
    // returns: size of new selection

    QString fileWithPath(const QString &fileName);
    // returns absolute canoncial path and filename to given filename string

    QString pathToFile(const QString &fileName);
    // returns absolute canoncial path without filename to given filename string

    QString pathReplaced(const QString &fileName, const QString &newPath);
    // extracts just name from fileName, and returns that prepended with absolute newPath

    QString textShortened(const QString &text, int left, int right);
    // if text is longer then left+5+right, returns copy with chars from middle replaced with " ... "

    QString &replaceUnicodeSeparators(QString &str);
    // replaces all unicode line and paragrapsh separator characters with char '\n'

    bool isBlankLine(const QString &line);
    // returns: true if line doesn't contain any non-whitespace characters.

    bool isAfterEscape(QTextCursor cur);
    // returns: true if there is an odd number of backslashes before cur

    bool autoSelectExpression(QTextCursor &cur, const QString &lang = QString());
    // selects expression backwards from cur, returns with cur containing selection
    // cur: text cursor to use as starting point, and for returning the selection
    // lang: programming language, not used yet, assumes Ruby
    // returns: true if selection successful

    bool autoSelectQuoted(QTextCursor &cur);
    // selects the quoted string (either with double or single quotes) surrounding cur
    // cur: text cursor to use as starting point, and for returning the selection
    // returns: true if selection successful

    char getPair(char ch);
    // returns: pair of ch, or ch itself for pairless characters. paired: (){}[]<>

    bool findNestedPairForward(char start, QTextCursor &cursor);
    // finds and selectes pair of delimiter char for other, going forward from position of cur
    // pair: character for which the pair is to be found
    // cur: on success will contain the found pair as selection
    // returns: true on success (cur modified), false on failure (cur unmodified)

    bool findNestedPairBack(char end, QTextCursor &cursor);
    // finds and selectes pair of delimiter char for other, going back from position of cur
    // pair: character for which the pair is to be found
    // cur: on success will contain the found pair as selection
    // returns: true on success (cur modified), false on failure (cur unmodified)

    bool findNestedPair(char pair, QTextCursor &cursor);
    // finds and selectes pair of delimiter char for other, going back or forward depending on char
    // pair: character for which the pair is to be found
    // cur: on success will contain the found pair as selection
    // returns: true on success (cur modified), false on failure (cur unmodified)

    bool blockDelimiterOrWordUnderCursor(QTextCursor &cursor);
    // finds and selects block keyword or block delimiter char under or before cursor
    // cur: starting point, on success returns with delimiter selected
    // returns: true on success (cur modified), false on failure (cur unmodified)

}


#endif // TDRIVER_EDITOR_COMMON_H

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


#include "tdriver_editor_common.h"

#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QRegExp>
#include <QFileInfo>
#include <QSettings>
#include <QApplication>
#include <QTextCursor>
#include <QSet>

#include <tdriver_debug_macros.h>

/**************************************************************
  * Debug functions and globals
  **************************************************************/

QSettings *MEC::settings = NULL;


void MEC::dumpStdItem(int indent, const QStandardItem *item)
{
    QString indstr(indent*2, ' ');

    if (!item) {
        qDebug() << indstr + "NULL ITEM!";
        return;
    }
    qDebug() << indstr + item->text()+"\t" << item->data(Qt::DisplayRole) << item->data(Qt::EditRole);
    for (int row=0 ; row < item->rowCount(); ++row) {
        dumpStdItem(indent+1, item->child(row));
    }
}


void MEC::dumpStdModel(const QStandardItemModel *model)
{
    qDebug() << "dumpStdModel" << model->children().size() << model->rowCount() << model->columnCount();
    for (int row=0 ; row < model->rowCount(); ++row) {
        dumpStdItem(1, model->item(row));
    }
}


QString MEC::dumpBreakpoint(const struct MEC::Breakpoint *bpp)
{
    return QString("#%1 %2 %3:%4").
            arg(bpp->num).
            arg(bpp->enabled ? "y" : "n").
            arg(bpp->file).
            arg(bpp->line);
}


QString MEC::dumpBreakpointList(const QList<struct MEC::Breakpoint> &bplist, const QString prefix, const QString delim, const QString suffix)
{
    MEC::Breakpoint bp;
    QStringList bps;

    foreach(bp, bplist)
        bps.append(dumpBreakpoint(&bp));

    return prefix + bps.join(delim) + suffix;
}


/**************************************************************
  * TDriver Editor Definition File handling
  **************************************************************/

MEC::DefinitionFileType MEC::getDefinitionFile(const QString &fileName, QStringList &contents, QMap<QString, QString> *defsPtr)
{
    QFile file(fileName);

    MEC::DefinitionFileType type = InvalidDefinitionFile;

    file.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!file.isReadable() ||
            (type = MEC::getDefinitionFile(file, contents, defsPtr)) == InvalidDefinitionFile) {
        qWarning() << "Failed to read from file " << file.fileName();
    }
    //else qDebug() << FFL << "got file type" << type << "completion count" << contents.size();

    return type;
}


MEC::DefinitionFileType MEC::getDefinitionFile(QFile &file, QStringList &contents, QMap<QString, QString> *defsPtr)
{
    Q_ASSERT (file.openMode() & QIODevice::Text);

    QByteArray line1(file.readLine());
    QRegExp sepExp;

    // determine file type
    DefinitionFileType type = parseDefinitionFileType(line1, sepExp, defsPtr);
    if (type == InvalidDefinitionFile) {
        qWarning() << file.fileName() << "invalid first line";
        qDebug() << FFL << line1;
        return type;
    }

    // read the file
    QString tempStr(file.readAll());
    if (type == PlainTextFile) {
        //qDebug() << FFL << "PlainTextFile: prepending first line" << tempStr;
        tempStr.prepend(line1);
    }

    // split the file using sepExp, which is also supposed to remove whitespace at the start of each Definition
    //qDebug() << FFL << "SPLITTING WITH" << sepExp.pattern();
    contents = tempStr.split(sepExp, QString::SkipEmptyParts);
    contents.sort();
    contents.removeDuplicates();
    return type;
}


static inline bool isValidDefinitionSeparator(const QString &str)
{
    static const QRegExp regExp("[]{}\\^$*+?.]");
    //qDebug() << FFL << str << regExp.pattern();
    if (str.isEmpty()) return false;
    if(str.contains(regExp)) return false;
    return true;
}


MEC::DefinitionFileType MEC::parseDefinitionFileType(const QByteArray &line1, QRegExp &separator, QMap<QString,QString> *defsPtr)
{

    QString lineStr = line1.simplified();

    QString sepStr("(\\n)+\\s*"); // default value matches one or more line breaks, then whitespace at the start of next line
    QString typeStr;
    DefinitionFileType type;

    if (!lineStr.startsWith("TDriver Editor Definition File:")) {
        // use default sepStr
        type = PlainTextFile;
    }
    else {
        static QMap<QString,QString> localDefs;
        if (!defsPtr) {
            localDefs.clear();
            defsPtr = &localDefs;
        }

        static const QRegExp defExp("\\b(\\w+)=(\\S*)");

        for(int pos = 0; (pos = defExp.indexIn(lineStr, pos)) != -1; pos += defExp.matchedLength()) {
            if (defExp.capturedTexts().size() == 1+2) {
                defsPtr->insert(defExp.cap(1).toUpper(), defExp.cap(2));
                //qDebug() << FFL << defExp.cap(1) << defs.value(defExp.cap(1)) << defExp.cap(2);
            }
            else qDebug() << FFL << "IGNORED funny capture:" << pos << lineStr << defExp.capturedTexts();
        }

        typeStr = defsPtr->value("TYPE").toUpper();

        if (typeStr == "SINGLELINE") {
            // use default sepStr
            type = SimpleOneLineDefinitionList;
        }
        else if (typeStr == "MULTILINE") {
            // build new sepStr
            sepStr = defsPtr->value("LINESEPARATOR");
            if (sepStr.isEmpty() || !isValidDefinitionSeparator(sepStr)) {
                qDebug() << FFL << "Invalid first line: bad or missing separator" << sepStr;
                return InvalidDefinitionFile;
            }
            sepStr.prepend("\\n"); // require single LF just before separator
            sepStr.append("[^\\n]*\\n\\s*"); // ignore rest of line, then match empty whitespace at the start of next line

            type = SimpleMultiLineDefinitionList;
        }
        else {
            qDebug() << FFL << "Invalid first line: bad or missing type" << typeStr;
            return InvalidDefinitionFile;
        }
    }

    if (type != InvalidDefinitionFile) {
        //qDebug() << FFL << "Got type and regexp" << typeStr << sepStr;
        separator.setPattern(sepStr);
        separator.setPatternSyntax(QRegExp::RegExp);
    }
    return type;
}


/**************************************************************
  * Miscellaneous helper functions
  **************************************************************/

int MEC::modifySelectionEnd(QTextCursor &cursor, const QString &text, int startPos)
{
    QString seltext(cursor.selectedText());
    if (startPos >= 0) seltext.truncate(startPos);

    QString newtext(seltext + text);

    // replace selected text with appended text and re-select
    int pos = cursor.selectionStart();
    cursor.insertText(newtext);
    cursor.setPosition(pos, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, newtext.size());
    return newtext.size();
}


QString MEC::absoluteFilePath(const QString &fileName)
{
    QFileInfo info(fileName);
    return info.absoluteFilePath();
}


QString MEC::pathReplaced(const QString &fileName, const QString &newPath)
{
    QFileInfo info(fileName);
    info.setFile(newPath + "/" + info.fileName());
    return info.absoluteFilePath();
}


QString MEC::textShortened(const QString &text, int left, int right)
{
    if (text.size() <= left+5+right)
        return text; // no need to shorten
    else
        return text.left(left) + " ... " + text.right(right);
}


QString &MEC::replaceUnicodeSeparators(QString &str)
{
    // LineSeparator replaced with ParagraphSeparator and not '\n' first,
    // because they have same byte size,
    // so this will hopefully avoid entire string getting shifted twice.
    return str
            .replace(QChar::LineSeparator, QChar::ParagraphSeparator)
            .replace(QChar::ParagraphSeparator, "\n");
}


bool MEC::isBlankLine(const QString &line)
{
    const QChar *cp = line.constData();
    unsigned count = line.length();
    while (count--) {
        if (!cp->isSpace()) return false;
        ++cp;
    }
    return true;
}


bool MEC::isAfterEscape(QTextCursor cur)
{
    // return true if there is an odd number of backslashes before cursor selection end
    if (cur.isNull()) return false;

    cur.setPosition(cur.selectionEnd());

    int count = 0;
    while (cur.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor) &&
           cur.selectedText() == "\\") {
        count++;
        cur.clearSelection();
    }

    return ((count & 1)==1);
}


struct AutoSelectLists {
    QSet<QChar> NormEnd;
    QSet<QChar> QuotStart;
    QSet<QChar> DotChars;

    // ParePairs MUST contain all PareRight values as keys
    QMap<QChar, QChar> ParePairs;
    QSet<QChar> PareRight;
    QSet<QChar> PareLeft;

    AutoSelectLists() {
        QuotStart << '\'' << '"';
        DotChars << '.' << ':';

        ParePairs[')'] = '(';
        ParePairs['}'] = '{';
        ParePairs[']'] = '[';

        foreach (QChar ch, ParePairs.keys()) {
            NormEnd << ch;
            PareRight << ch;
        }
        foreach (QChar ch, ParePairs.values()) {
            NormEnd << ch;
            PareLeft << ch;
        }
    }
};


static AutoSelectLists asl;

bool MEC::autoSelectExpression(QTextCursor &cur, const QString &lang)
{
    if (!lang.isNull()) {
        qWarning() << FFL << "lang argument not supported yet, ignored!";
    }

    int validCount = 0;
    // count characters to include in autoselection

    QChar lastCh(QChar::Null);
    QTextCursor tmpCur(cur);
    enum { NORM, DOT, QUOT, PARE, PAREQUOT } mode, lastMode;
    mode = lastMode = NORM;
    QChar quotStarter(QChar::Null);

    int pareCount = 0;
    QChar pareStarter(QChar::Null);

    forever {
        if (!tmpCur.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor)) break;
        QChar ch = tmpCur.selectedText().at(0);

        //qDebug() << FFL << "chs:" << ch << lastCh << "mode:" << mode << lastMode << "misc:" << quotStarter << pareCount << pareStarter;

        if (mode==NORM && asl.NormEnd.contains(ch)) {
            // normal mode break that must not happen in DOT mode
            break;
        }

        else if ((lastMode == QUOT || lastMode == PAREQUOT) &&
                 mode != lastMode &&
                 (ch == '\\' && lastCh == quotStarter)) {
            // escaped quote char, restore previous mode
            mode = lastMode;
        }

        else if (mode==NORM || mode==DOT) {
            if (asl.DotChars.contains(ch)) {
                // switch to/keep DOT mode
                lastMode = mode, mode = DOT;
            }
            else if (asl.QuotStart.contains(ch)) {
                // switch to QUOT mode
                quotStarter = ch;
                lastMode = mode, mode = QUOT;
            }
            else if (asl.PareRight.contains(ch)) {
                // switch to PARE mode
                pareCount = 1;
                pareStarter = ch;
                lastMode = mode, mode = PARE;
            }
            else if (ch.isSpace() || asl.PareLeft.contains(ch)) {
                // whitespace or open parenthesis ends selection (invalid if dot mode)
                break;
            }
            else if (mode == DOT ) {
                // switch to NORM mode
                lastMode = mode, mode = NORM;
            }
            // else continue NORM mode
        }

        else if (mode==QUOT || mode == PAREQUOT) {
            if (ch == quotStarter) {
                // end QUOT mode, either to PARE or NORM
                lastMode = mode;
                mode = (mode==PAREQUOT) ? PARE : NORM;
            }
            // else continue QUOT mode
        }

        else if (mode==PARE) {
            if (ch == asl.ParePairs.value(pareStarter)) {
                // decrease depth of PARE mode or switch it off
                --pareCount;
                if (pareCount <= 0) {
                    lastMode = mode, mode = NORM;
                }
            }
            else if (ch == pareStarter) {
                // increase depth of pare mode
                ++pareCount;
            }
            // else continue PARE mode, other parenthesis types are just ignored
        }

        lastCh = ch;
        validCount++;
        tmpCur.clearSelection();
    }

    //qDebug() << FFL << "FINAL: lch:" << lastCh << "mode:" << mode << lastMode << "misc:" << quotStarter << pareCount << pareStarter;
    if  (validCount <= 0 || lastCh.isNull() || mode != NORM) {
        qDebug() << FFL << "returning false, reason:" << validCount << lastCh << mode;
        return false;
    }

    // do selection, put cursor to selection start
    cur.clearSelection();
    cur.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, validCount);

    return true;
}


bool MEC::autoSelectQuoted(QTextCursor &cur)
{
    // TODO: handle multiline strings: find start of string in earlier line, and find end of string in later line

    int startPos = cur.position();
    cur.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);
    int endLinePos = cur.position();
    cur.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);

    QString foundChr; // emtpy string means not found
    int foundStartPos = startPos;
    bool escape = false;
    bool justClosed = false; // justClosed is needed to capture end quote just before cursor pos

    // find beginning quote char before current cursor position
    while (cur.position() < startPos) {
        cur.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        if (justClosed) {
            // a quoted string was closed before above cursor move, so reset found status
            foundChr.clear();
            foundStartPos = startPos;
            justClosed = false;
        }
        QString chr = cur.selectedText().right(1);
        if (!escape && chr == "\\") {
            escape = true;
        }
        else {
            if (foundChr.isEmpty() && !escape && (chr=="\"" || chr=="'")) {
                foundChr = chr;
                foundStartPos = cur.position();
            }
            else if (!foundChr.isEmpty() && !escape && chr==foundChr) {
                // justClosed mechanism is needed to select quoted stringe preceding cursor pos
                justClosed = true;
            }
            escape = false;
        }
        if (foundChr.isEmpty()) {
            cur.clearSelection();
        }
    }

    if (!justClosed && cur.hasSelection()) {
        // quoted string didn't close at cursor position,
        // and start of string has been selected,
        // find end of quoted string (or end of line
        while (cur.position() < endLinePos) {
            cur.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            QString chr = cur.selectedText().right(1);

            if (!escape && chr == "\\") {
                escape = true;
            }
            else {
                if (!escape && chr == foundChr) break; // found matching quote!
                escape = false;
            }
        }
    }
    return cur.hasSelection();
}


char MEC::getPair(char ch)
{
    switch(ch) {

    case '(': return ')';
    case '[': return ']';
    case '{': return '}';
    case '<': return '>';

    case ')': return '(';
    case ']': return '[';
    case '}': return '{';
    case '>': return '<';

    default: return ch;
    }
}


bool MEC::findNestedPairForward(char startCh, QTextCursor &cursor)
{
    int depth = 0;
    char endCh = getPair(startCh);

    QTextCursor cur(cursor);
    if (cur.hasSelection()) {
        cur.setPosition(cur.selectionEnd());
    }

    while(cur.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor)) {

        char ch = cur.selectedText().at(0).toAscii();
        if (ch == startCh) {
            ++depth;
        }
        else if (ch == endCh) {
            if (depth > 0) {
                --depth;
            }
            else {
                cursor = cur;
                return true;
            }
        }
        cur.clearSelection();
    }
    return false;
}


bool MEC::findNestedPairBack(char endCh, QTextCursor &cursor)
{
    int depth = 0;
    char startCh = getPair(endCh);

    QTextCursor cur(cursor);
    if (cur.hasSelection()) {
        cur.setPosition(cur.selectionStart());
    }

    while(cur.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor)) {

        char ch = cur.selectedText().at(0).toAscii();
        if (ch == endCh) {
            ++depth;
        }
        else if (ch == startCh) {
            if (depth > 0) {
                --depth;
            }
            else {
                cursor = cur;
                return true;
            }
        }
        cur.clearSelection();
    }
    return false;
}


bool MEC::findNestedPair(char pair, QTextCursor &cursor)
{
    char other = getPair(pair);
    if (other < pair)
        return findNestedPairBack(pair, cursor);
    if (other > pair)
        return findNestedPairForward(pair, cursor);
    else
        return false;
}


bool MEC::blockDelimiterOrWordUnderCursor(QTextCursor &cursor)
{
    {
        QTextCursor cur(cursor);

        // don't highlight pairs if there's an active selection
        if (cur.hasSelection()) return false;

        // don't highlight pairs if moving to left fails (at start of document)
        if (!cur.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor)) return false;

        QChar selChar = cur.selectedText().at(0);
        if (!selChar.isLetter()) {
            static const QSet<QChar> blockChars(QSet<QChar>() << '{' << '[' << '(' << '}' << ']' << ')');
            if(blockChars.contains(selChar)) {
                cursor = cur;
                return true;
            }
            else {
                // non-letter, non-brace character, no need to highlight
                return false;
            }
        }
        // else was letter, see if there will be a word to highlight
        Q_ASSERT(selChar.isLetter());
    }

#if 1
    // handle words, UNTESTED CODE

    QTextCursor cur(cursor);
    int startPos = cur.position();
    cur.movePosition(QTextCursor::WordLeft);
    while(cur.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor)) {
        QChar selChar = cur.selectedText().right(1).at(0);

        if (selChar.isLetter()) continue; // ok, keep going
        else if (selChar.isNumber()) return false; // block keywords can't have numbers
        else if (cur.position() <= startPos) return false; // didn't select up to original pos
        else {
            // unselect last selected and break for success
            cur.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
            break;
        }
    }
    if (cur.selectedText().size() >= 2) {
        // assume shortest block keyword is 2 chars
        cursor = cur;
        return true;
    }
#else
    // word highlight not implemented
#endif
    return false;

}

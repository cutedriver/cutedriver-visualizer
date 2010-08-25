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


#ifndef TDRIVER_HIGHLIGHTER_H
#define TDRIVER_HIGHLIGHTER_H


#include <QSyntaxHighlighter>

class QString;
class QTextCharFormat;

#include <QList>

class TDriverHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

protected:
    // Inner classes for highlight rules

    // Virtual base class for rule entries
    class HighlightingRuleBase
    {
    public:
        virtual int type() const =0;
        QRegExp *matchPat;
        QRegExp::CaretMode matchCaretMode;
        const QTextCharFormat *format;
        // owner: will be modified by const methods
        const TDriverHighlighter *owner;
        const int stateIndex;
        bool resetBlockState;
        // handlePreviousState: called when previous block state is index of this rule
        // returns length of text it highlighted at the start of text
        virtual int handlePreviousState(const QString &) const {
            qWarning("TDriverHighlighter::HighlightingRuleBase::handlePreviousState called!");
            return 0;
        }
        // postMatch: called when rule matched
        // returns new currentBlockState >= -1, or -2 for no change in state
        virtual void postMatch(const QString &text, int &startInd, int &formatLen) const;

        HighlightingRuleBase(TDriverHighlighter *owner);
    };


    // Class for single line single rule entries, just redefines type()
    class HighlightingRule1: public HighlightingRuleBase
    {
    public:
        virtual int type() const { return 1; };
        HighlightingRule1(TDriverHighlighter *owner) : HighlightingRuleBase(owner) {
        }
    };


    // Class for two rule, possibly multi rules entries
    class HighlightingRule2: public HighlightingRuleBase
    {
    public:
        virtual int type() const { return 2; };
        QRegExp *endPat;
        QRegExp::CaretMode endCaretMode;
        // handlePreviousState: overloaded
        virtual int handlePreviousState(const QString &text) const;
        // postMatch: overloaded
        virtual void postMatch(const QString &text, int &startInd, int &formatLen) const;
        HighlightingRule2(TDriverHighlighter *owner);
    };


public:
    explicit TDriverHighlighter(QTextDocument *parent=NULL);

    // returns -1 for file error, -2 for parse error, >=0 for number of rules added
    int readPlainStrings(const QString &file, const QTextCharFormat *format,
                            QList<const HighlightingRuleBase*> &rules);

protected:
    virtual void highlightBlock(const QString &);

    // standard Formats, optionally modified in derived classes
    QTextCharFormat *defaultFormat; // has special meaning
    QTextCharFormat *keywordFormat;
    QTextCharFormat *singleQuotationFormat;
    QTextCharFormat *doubleQuotationFormat;

    // fortdriverng stuff


    QList<HighlightingRuleBase*> stateRulePtrs;
    QList< QList<const HighlightingRuleBase*> > ruleListList;
};

#endif // TDRIVER_HIGHLIGHTER_H

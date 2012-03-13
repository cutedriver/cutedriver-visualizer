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


#include "tdriver_rubyhighlighter.h"
#include <tdriver_util.h>

#include <QTextCharFormat>
#include <QFont>
#include <QStringList>
#include <QFile>
#include <QSettings>

#include "tdriver_editor_common.h"

TDriverRubyHighlighter::TDriverRubyHighlighter(QTextDocument *parent) :
        TDriverHighlighter(parent),
        rubyMethodFormat(new QTextCharFormat),
        rubyClassFormat(new QTextCharFormat),
        rubySymbolFormat(new QTextCharFormat),
        rubyCommentFormat(new QTextCharFormat),
        rubyClassVariableFormat(new QTextCharFormat)
{
    QList<const HighlightingRuleBase*> rules;

    rubyMethodFormat->setForeground(Qt::darkRed);

    rubyClassFormat->setForeground(Qt::darkRed);
    rubyClassFormat->setFontWeight(QFont::Bold);

    rubySymbolFormat->setForeground(Qt::darkCyan);

    rubyCommentFormat->setForeground(Qt::gray);

    rubyClassVariableFormat->setForeground(Qt::darkGreen);

    /*
       Note: order of highlight rule list lists is:
       - block rules that must start at start of line
       - block rules with start and end (this is in the list already by parent class, and is appended to),
       - comment rules that go until end of line,
       - ruby symbols starting with :
       - keywords and reserved words
    */

    // ruby block/documentation comment
    {
        HighlightingRule2 *rule2 = new HighlightingRule2(this);
        // TODO: check if it's really true that these both must be at start of line
        rule2->matchPat = new QRegExp("^=begin");
        rule2->endPat = new QRegExp("^=end");
        rule2->matchCaretMode = rule2->endCaretMode = QRegExp::CaretAtZero;
        rule2->format = rubyCommentFormat;
        rules.append(rule2);
    }
    ruleListList.prepend(rules);
    rules.clear();

    // ruby %-expressions, partial and inefficient implementation,
    // TODO: matchPat should be something like %([qQwW])(.) and change endPat dynamically
    // TODO: probably need to create a new rule for this
    {
        HighlightingRule2 *rule2;

        rule2 = new HighlightingRule2(this);
        rule2->format = singleQuotationFormat;
        rule2->matchPat = new QRegExp("%q\\[");
        rule2->endPat = new QRegExp("(^'|[^\\\\](\\\\\\\\)*\\])");
        rules.append(rule2);

        rule2 = new HighlightingRule2(this);
        rule2->format = singleQuotationFormat;
        rule2->matchPat = new QRegExp("%q\\{");
        rule2->endPat = new QRegExp("(^'|[^\\\\](\\\\\\\\)*\\})");
        rules.append(rule2);

        rule2 = new HighlightingRule2(this);
        rule2->format = singleQuotationFormat;
        rule2->matchPat = new QRegExp("%q\\(");
        rule2->endPat = new QRegExp("(^'|[^\\\\](\\\\\\\\)*\\)");
        rules.append(rule2);

        rule2 = new HighlightingRule2(this);
        rule2->format = doubleQuotationFormat;
        rule2->matchPat = new QRegExp("%Q\\[");
        rule2->endPat = new QRegExp("(^'|[^\\\\](\\\\\\\\)*\\])");
        rules.append(rule2);

        rule2 = new HighlightingRule2(this);
        rule2->format = doubleQuotationFormat;
        rule2->matchPat = new QRegExp("%Q\\{");
        rule2->endPat = new QRegExp("(^'|[^\\\\](\\\\\\\\)*\\})");
        rules.append(rule2);

        rule2 = new HighlightingRule2(this);
        rule2->format = doubleQuotationFormat;
        rule2->matchPat = new QRegExp("%Q\\(");
        rule2->endPat = new QRegExp("(^'|[^\\\\](\\\\\\\\)*\\)");
        rules.append(rule2);
    }
    // append to list that comes from parent class
    ruleListList.last().append(rules);
    rules.clear();

    // until-end-of-line blocks, like #-comment blocks
    {
        HighlightingRule1 *rule1 = new HighlightingRule1(this);
        rule1->matchPat = new QRegExp("#.*");
        rule1->format = rubyCommentFormat;
        rule1->resetBlockState = true;
        rules.append(rule1);
    }
    ruleListList.append(rules);
    rules.clear();

    // ruby symbols
    // TODO: make it possible to have rule that highlights colon differently from symbol word
    {
        HighlightingRule1 *rule1 = new HighlightingRule1(this);
        rule1->matchPat = new QRegExp(":\\w*");
        rule1->format = rubySymbolFormat;
        rules.append(rule1);
    }
    ruleListList.append(rules);
    rules.clear();

    // ruby class variables
    {
        HighlightingRule1 *rule1 = new HighlightingRule1(this);
        rule1->matchPat = new QRegExp("@\\w*");
        rule1->format = rubyClassVariableFormat;
        rules.append(rule1);
    }
    ruleListList.append(rules);
    rules.clear();

    // ruby keywords read form files
    {
        QString dir = TDriverUtil::tdriverHelperFilePath("completions/");
        readPlainStrings(dir+"plain_ruby_keywords.txt", keywordFormat, rules);
        //readPlainStrings(dir+"plain_ruby_methods.txt", rubyMethodFormat, rules);
        readPlainStrings(dir+"plain_ruby_classes.txt", rubyClassFormat, rules);
    }
    ruleListList.append(rules);
    rules.clear();
}


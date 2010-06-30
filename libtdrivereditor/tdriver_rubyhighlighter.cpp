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
        rubyCommentFormat(new QTextCharFormat)
{
    QList<const HighlightingRuleBase*> rules;

    rubyMethodFormat->setForeground(Qt::darkRed);

    rubyClassFormat->setForeground(Qt::darkRed);
    rubyClassFormat->setFontWeight(QFont::Bold);

    rubySymbolFormat->setForeground(Qt::darkYellow);

    rubyCommentFormat->setForeground(Qt::darkGreen);

    // Note: order of highlight rules is arranged so that "area rules" like comments and quotes
    // should be before smaller rules like keywords, for performance reasons, to avoid highlighting
    // keywords etc twice when they're inside comments, quotes etc.

    // create a new list for until-end-of-line comments and put it even before superclass default rules
    {
        HighlightingRule1 *rule1 = new HighlightingRule1(this);
        rule1->matchPat = new QRegExp("#.*");
        rule1->format = rubyCommentFormat;
        rules.append(rule1);
    }
    ruleListList.prepend(rules);

    // create ruby rules to go after the superclass default rules
    rules.clear();

    // ruby symbols
    // TODO: make it possible to have rule that highlights colon differently from symbol word
    {
        HighlightingRule1 *rule1 = new HighlightingRule1(this);
        rule1->matchPat = new QRegExp(":\\w*");
        rule1->format = rubySymbolFormat;
        rules.append(rule1);
    }

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


    QString dir = TDriverUtil::tdriverHelperFilePath("completions/");

    // read highlight words from a few files
    readPlainStrings(dir+"plain_ruby_keywords.txt", keywordFormat, rules);

    //readPlainStrings(dir+"plain_ruby_methods.txt", rubyMethodFormat, rules);

    readPlainStrings(dir+"plain_ruby_classes.txt", rubyClassFormat, rules);

    // put these after default multiline quotes
    ruleListList.append(rules);
    //rules.clear();
}


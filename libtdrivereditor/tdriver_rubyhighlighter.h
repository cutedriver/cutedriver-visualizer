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


#ifndef TDRIVER_RUBYHIGHLIGHTER_H
#define TDRIVER_RUBYHIGHLIGHTER_H

#include "libtdrivereditor_global.h"

#include "tdriver_highlighter.h"

class LIBTDRIVEREDITORSHARED_EXPORT TDriverRubyHighlighter : public TDriverHighlighter
{
    Q_OBJECT
public:
    explicit TDriverRubyHighlighter(QTextDocument *parent=NULL);


protected:
    QTextCharFormat *rubyMethodFormat;
    QTextCharFormat *rubyClassFormat;
    QTextCharFormat *rubySymbolFormat;
    QTextCharFormat *rubyCommentFormat;
};

#endif // TDRIVER_RUBYHIGHLIGHTER_H

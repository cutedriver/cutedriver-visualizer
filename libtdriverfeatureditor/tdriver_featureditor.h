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

#ifndef TDRIVER_FEATUREDITOR_H
#define TDRIVER_FEATUREDITOR_H

#include <QWidget>

#include "libtdriverfeatureditor_global.h"

class TDriverFeaturAbstractView;

class LIBTDRIVERFEATUREDITORSHARED_EXPORT TDriverFeaturEditor : public QWidget
{
    Q_OBJECT

public:
    TDriverFeaturEditor(QWidget *parent = 0);

private:
    TDriverFeaturAbstractView *featureList;
    TDriverFeaturAbstractView *scenarioList;
    TDriverFeaturAbstractView *scenarioStepList;

    TDriverFeaturAbstractView *stepDefinitionList;
    TDriverFeaturAbstractView *stepFileList;
};

#endif // TDRIVER_FEATUREDITOR_H

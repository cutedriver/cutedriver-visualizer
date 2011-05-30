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

#include "tdriver_featureditor.h"

#include "tdriver_featurfeatureview.h"
#include "tdriver_featurscenarioview.h"
#include "tdriver_featurscenariostepview.h"
#include "tdriver_featurstepdefview.h"
#include "tdriver_featurstepfileview.h"

#include <tdriver_debug_macros.h>

#include <QtGui>


TDriverFeaturEditor::TDriverFeaturEditor(QWidget *parent) :
    QWidget(parent)
  , featureList(new TDriverFeaturFeatureView)
  , scenarioList(new TDriverFeaturScenarioView)
  , scenarioStepList(new TDriverFeaturScenarioStepView)
  , stepDefinitionList(new TDriverFeaturStepDefView)
  , stepFileList(new TDriverFeaturStepFileView)
{
    setLayout(new QVBoxLayout());

    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    layout()->addWidget(splitter);

    splitter->addWidget(featureList);
    splitter->addWidget(scenarioList);
    splitter->addWidget(scenarioStepList);
    splitter->addWidget(stepDefinitionList);
    splitter->addWidget(stepFileList);

    connect(featureList->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            scenarioList, SLOT(resetPathFromIndex(QModelIndex)));
    connect(featureList, SIGNAL(reScanned(QString)),
            stepFileList, SLOT(resetPath(QString)));
    connect(featureList->view(), SIGNAL(doubleClicked(QModelIndex)),
            SLOT(editFromIndex(QModelIndex)));

    connect(scenarioList->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            scenarioStepList, SLOT(resetPathFromIndex(QModelIndex)));
    connect(scenarioList, SIGNAL(reScanned(QString)),
            scenarioStepList, SLOT(clearView()));
    connect(scenarioList->view(), SIGNAL(doubleClicked(QModelIndex)),
            SLOT(editFromIndex(QModelIndex)));

    connect(scenarioStepList->view(), SIGNAL(doubleClicked(QModelIndex)),
            SLOT(editFromIndex(QModelIndex)));

    connect(stepDefinitionList->view(), SIGNAL(doubleClicked(QModelIndex)),
            SLOT(editFromIndex(QModelIndex)));

    connect(stepFileList->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            stepDefinitionList, SLOT(resetPathFromIndex(QModelIndex)));
    connect(stepFileList->view(), SIGNAL(doubleClicked(QModelIndex)),
            SLOT(editFromIndex(QModelIndex)));

    // debug kickstart
    featureList->setPath(QDir::homePath() + "/tdriver/tests/test/features");
    featureList->setPendingScan();

    //connect(this, SIGNAL(fileChangeRelay(QString)), featureList, SLOT(aFileChanged(QString)));
    connect(this, SIGNAL(fileChangeRelay(QString)), scenarioList, SLOT(aFileChanged(QString)));
    connect(this, SIGNAL(fileChangeRelay(QString)), scenarioStepList, SLOT(aFileChanged(QString)));
    //connect(this, SIGNAL(fileChangeRelay(QString)), featureList, SLOT(aFileChanged(QString)));
    connect(this, SIGNAL(fileChangeRelay(QString)), stepFileList, SLOT(aFileChanged(QString)));
}


void TDriverFeaturEditor::editFromIndex(const QModelIndex &index)
{
    QString path;
    const QAbstractItemModel *model = index.model();

    if (index.isValid() && model) {
        path = model->data(index, TDriverFeaturAbstractView::ActualPathRole).toString();
        qDebug() << FCFL << "emitting fileEditRequest with path from model:" << path;
        emit fileEditRequest(path);
    }
    else {
        qDebug() << FCFL << "could not emit fileEditRequest";
    }
}

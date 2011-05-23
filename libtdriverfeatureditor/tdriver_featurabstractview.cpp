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

#include "tdriver_featurabstractview.h"

#include <tdriver_debug_macros.h>

#include <QtGui>

TDriverFeaturAbsractView::TDriverFeaturAbsractView(const QString &title, QWidget *parent) :
    QWidget(parent)
  , _toolBar(new QToolBar(title))
  , _listView(new QListView)
  , _locationBox(new QComboBox)
  //, refreshAct(new QAction(tr("Refresh")), this)
{

    _locationBox->setEditable(true);
    _locationBox->setDuplicatesEnabled(false);
    _locationBox->setInsertPolicy(QComboBox::InsertAtTop);
    connect(_locationBox, SIGNAL(activated(int)), SLOT(pullPath()));

    setLayout(new QVBoxLayout);
    _toolBar->addWidget(new QLabel(title));
    layout()->addWidget(_toolBar);
    layout()->addWidget(_locationBox);
    layout()->addWidget(_listView);

    setModel(new QStandardItemModel(this));
    //bar->addAction(refreshAct);
}

void TDriverFeaturAbsractView::setModel(QAbstractItemModel *model)
{
    _listView->setModel(model);
}

QAbstractItemModel * TDriverFeaturAbsractView::model()
{
    return _listView->model();
}


void TDriverFeaturAbsractView::clearModel()
{

    model()->removeRows(0, model()->rowCount()-1);
}




void TDriverFeaturAbsractView::pullPath()
{
    QString value = _locationBox->currentText();
    qDebug() << FCFL << value;
    if (!value.isEmpty()) {
        setPath(value);
        rescanPathWithPattern();
    }
}


int TDriverFeaturAbsractView::rescanPathWithPattern()
{
    if (!_listView->model()) return -1;

    qDebug() << FCFL << __path << __scanPattern;

    if (!QDir(__path).exists()) {
        return -1;
    }

    clearModel();

    QDirIterator dirIt(__path,
                       QStringList() << __scanPattern,
                       QDir::Files,
                       QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

    QList<QFileInfo> fileInfos;
    while (dirIt.hasNext()) {
        dirIt.next();
        fileInfos << dirIt.fileInfo();
    }

    qDebug() << FCFL << model()->rowCount() << model()->columnCount();
    if (!model()->insertRows(0, fileInfos.size())) {
        qWarning() << "Failed to insert" << fileInfos.size() << "rows to model at" << FCFL;
        return -2;
    }
    qDebug() << FCFL << model()->rowCount() << model()->columnCount();

    if (model()->columnCount() < 1) {
        if (!model()->insertColumn(0)) {
            qWarning() << "Failed to insert one column to model at" << FCFL;
        }
    }

    for (int row = 0; row < fileInfos.size(); ++row) {
        QModelIndex index = model()->index(row, 0);

        //qDebug() << FCFL << fileInfos.at(row).filePath();

        if (!model()->setData(index, fileInfos.at(row).baseName(), Qt::EditRole)) {
            qWarning() << "Failed to set DisplayRole for model row" << row;
        }

        if (!model()->setData(index, fileInfos.at(row).absoluteFilePath(), Qt::ToolTipRole)) {
            qWarning() << "Failed to set ToolTipRole for model row" << row;
        }
    }
    return model()->rowCount();
}


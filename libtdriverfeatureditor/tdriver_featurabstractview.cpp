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

#include "tdriver_standardfeaturmodel.h"

#include <tdriver_debug_macros.h>

#include <QtGui>

TDriverFeaturAbstractView::TDriverFeaturAbstractView(const QString &title, QWidget *parent) :
    QWidget(parent)
  , _toolBar(new QToolBar(title))
  , _styleDelegate(new QStyledItemDelegate(this))
  , __listView(new QListView)
  , __locationBox(new QComboBox)
  , __pathInfo(new QFileInfo)
  , __pathLine(-1)
  , __pendingScan(false)
  , __scanType(NoScan)
  //, refreshAct(new QAction(tr("Refresh")), this)
{

    __locationBox->setEditable(true);
    __locationBox->setDuplicatesEnabled(false);
    __locationBox->setInsertPolicy(QComboBox::InsertAtTop);
    connect(__locationBox, SIGNAL(activated(int)), SLOT(resetPathFromBox()));

    setLayout(new QVBoxLayout);
    _toolBar->addWidget(new QLabel(title));
    layout()->addWidget(_toolBar);
    layout()->addWidget(__locationBox);
    layout()->addWidget(__listView);

    setModel(new TDriverStandardFeaturModel(this));
    //connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), SLOT(resetPathFromIndex(QModelIndex)));

    //bar->addAction(refreshAct);
}


TDriverFeaturAbstractView::~TDriverFeaturAbstractView()
{
    delete __pathInfo;
}


QString TDriverFeaturAbstractView::path()
{
    return (__locationBox->currentText());
}


void TDriverFeaturAbstractView::setPath(const QString &path)
{
    // detect if path ends with line number
    QString tmpPath(path);
    bool hasLineNum = false;
    __pathLine = -1;
    int lastColonPos = path.lastIndexOf(':');
    if (lastColonPos > 0) {
        bool ok = false;
        int line = path.mid(lastColonPos+1).toInt(&ok);
        if (ok) {
            hasLineNum = true;
            tmpPath = path.left(lastColonPos);
            __pathLine = line;
        }
    }
    __pathInfo->setFile(tmpPath);
    QString suffix = (!hasLineNum && pathInfo().isDir()) ? QString("/") : QString();

    setLocationBox(path+suffix);
}


void TDriverFeaturAbstractView::setModel(QAbstractItemModel *model)
{
    __listView->setModel(model);
}


QAbstractItemModel * TDriverFeaturAbstractView::model()
{
    return __listView->model();
}


bool TDriverFeaturAbstractView::clearModel(int rows)
{
    bool ret = true;
    model()->removeRows(0, model()->rowCount());

    if (rows > 0 && !model()->insertRows(0, rows)) {
        qWarning() << "Failed to insert" << rows << "rows to model at" << FCFL;
        ret = false;
    }

    return ret;
}


QItemSelectionModel * TDriverFeaturAbstractView::selectionModel()
{
    return __listView->selectionModel();
}


QAbstractItemView *TDriverFeaturAbstractView::view()
{
    return __listView;
}


void TDriverFeaturAbstractView::aFileChanged(const QString &path)
{
    if (QFileInfo(path).canonicalFilePath() == pathInfo().canonicalPath()) {
        reScan();
    }
}


void TDriverFeaturAbstractView::resetPath(const QString &path)
{
    qDebug() << FCFL << path;
    if(path != __locationBox->currentText()) {
        setPath(path);
        reScan();
    }
}


void TDriverFeaturAbstractView::resetPathFromIndex(const QModelIndex &index)
{
    qDebug() << FCFL;

    QString path;

    const QAbstractItemModel *model = index.model();
    if (index.isValid() && model) {
        path = model->data(index, ActualPathRole).toString();
        qDebug() << FCFL << "got path from model:" << path;
    }
    setPath(path);
    reScan();
}

void TDriverFeaturAbstractView::resetPathFromBox()
{
    QString path = __locationBox->currentText();
    qDebug() << FCFL << path;

    setPath(path);
    reScan();
}


void TDriverFeaturAbstractView::clearView()
{
    clearModel();
    setLocationBox(QString());
}


int TDriverFeaturAbstractView::reScan()
{
    __pendingScan = false;
    int ret = 0;

    bool locBoxEnabled = __locationBox->isEnabled();
    if (locBoxEnabled) __locationBox->setEnabled(false);

    switch(__scanType) {

    case NoScan: ret = 0; break;

    case DirScan: ret = doDirScan(); break;

    case FileScan: ret = doFileScan(); break;

    case FileSectionScan: ret = doFileSectionScan(); break;
    }

    qDebug() << FCFL << __scanType << ret;

    __locationBox->setEnabled(locBoxEnabled);

    if (ret >= 0) {
        emit reScanned(__pathInfo->canonicalFilePath());
    }
    else {
        qDebug() << FCFL << "Error code" << ret << "with scan type" << __scanType;
    }

    return ret;
}


void TDriverFeaturAbstractView::setLocationBox(const QString &text)
{
    __locationBox->insertItem(0, text);
    __locationBox->setCurrentIndex(0);

    // remove duplicates
    for(int ii = __locationBox->count()-1; ii > 0 ; --ii) {
        if (__locationBox->itemText(ii) == text) {
            __locationBox->removeItem(ii);
        }
    }

}


void TDriverFeaturAbstractView::showEvent(QShowEvent *ev)
{
    QWidget::showEvent(ev);

    if (__pendingScan) {
        reScan();
    }
}


int TDriverFeaturAbstractView::doDirScan()
{
    QString path(pathInfo().canonicalFilePath());

    qDebug() << FCFL << path << __scanPattern;
    Q_ASSERT(__listView->model());

    if (!pathInfo().isDir()) {
        qDebug() << FCFL << "called when path not dir:" << path;
        return -1;
    }


    QDirIterator dirIt(path,
                       QStringList() << __scanPattern,
                       QDir::Files,
                       QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

    QList<QFileInfo> fileInfos;
    while (dirIt.hasNext()) {
        dirIt.next();
        fileInfos << dirIt.fileInfo();
    }

    if (!clearModel(fileInfos.size())) {
        qWarning() << "Failed to initialize empty model at" << FCFL;
        return -2;
    }


    for (int row = 0; row < fileInfos.size(); ++row) {
        QModelIndex index = model()->index(row, 0);

        //qDebug() << FCFL << fileInfos.at(row).filePath();

        if (!model()->setData(index,
                              fileInfos.at(row).baseName())) {
            qWarning() << "Failed to set default role for model row" << row;
        }

        if (!model()->setData(index,
                              fileInfos.at(row).canonicalFilePath(),
                              ActualPathRole)) {
            qWarning() << "Failed to set ActualPathRole for model row" << row;
        }

        if (!model()->setData(index,
                              fileInfos.at(row).canonicalFilePath(),
                              Qt::ToolTipRole)) {
            qWarning() << "Failed to set ToolTipRole for model row" << row;
        }
    }
    return model()->rowCount();
}


int TDriverFeaturAbstractView::doFileScan()
{
    QString path(pathInfo().canonicalFilePath());

    qDebug() << FCFL << path << __scanPattern;
    Q_ASSERT(__listView->model());


    if (!clearModel()) {
        qWarning() << "Failed to initialize empty model at" << FCFL;
        return -2;
    }

    if (!pathInfo().isFile() && !pathInfo().isReadable()) {
        qDebug() << FCFL << "called when path not readable file:" << path;
        return -1;
    }

    QFile file(path);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Failed to open" << path << "with error" << file.errorString() << "at" << FCFL;
        return -2;
    }

    QRegExp rx(__scanPattern);

    QByteArray line;
    int modelRow = model()->rowCount();
    int lineNum = 0;

    qDebug() << FCFL << rx.isValid() << rx.pattern();

    qDebug() << FCFL << "________START FILE READ LOOP________";

    while ((line = file.readLine()).size() > 0) {
        file.unsetError();
        ++lineNum;
        QString lineStr(QString::fromUtf8(line.trimmed()));
        int pos = rx.indexIn(lineStr);
        qDebug() << lineNum << '(' << pos << rx.captureCount() << ')' << ':' << line.trimmed();
        if (pos >= 0 && rx.captureCount() >= 1) {

            if (!model()->insertRow(modelRow)) {
                qWarning() << "Failed to insert to row number" << modelRow << "to model at" << FCFL;
                return -2;
            }

            QModelIndex index = model()->index(modelRow, 0);

            if (!model()->setData(index,
                                  rx.cap(1))) {
                qWarning() << "Failed to set default role for model row" << modelRow;
            }

            if (!model()->setData(index,
                                  QString("%1:%2").arg(pathInfo().canonicalFilePath(), QString::number(lineNum)),
                                  ActualPathRole)) {
                qWarning() << "Failed to set ActualPathRole for model row" << modelRow;
            }

            if (!model()->setData(index,
                                  QString("%1: %2").arg(QString::number(lineNum), lineStr),
                                  Qt::ToolTipRole)) {
                qWarning() << "Failed to set ToolTipRole for model row" << modelRow;
            }

            ++modelRow;
        }
    }

    qDebug() << FCFL << "________ END FILE READ LOOP ________";

    if (file.error() != QFile::NoError) {
        qDebug() << FCFL << "reading ended with readLine error" << file.errorString();
    }

    return model()->rowCount();
}



int TDriverFeaturAbstractView::doFileSectionScan()
{

    QString path(pathInfo().canonicalFilePath());

    qDebug() << FCFL << path << scanPattern();
    Q_ASSERT(__listView->model());

    if (!pathInfo().isFile() && !pathInfo().isReadable()) {
        qDebug() << FCFL << "called when path not readable file:" << path;
        return -1;
    }

    QFile file(path);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Failed to open" << path << "with error" << file.errorString() << "at" << FCFL;
        return -2;
    }

    if (!clearModel()) {
        qWarning() << "Failed to initialize empty model at" << FCFL;
        return -2;
    }

    QRegExp rx(scanPattern());

    QByteArray line;
    int modelRow = model()->rowCount();
    int lineNum = 0;
    bool sectionOver = false;
    int pathLineNum = pathLineNumber();
    if (pathLineNum <= 0) pathLineNum = 1;

    qDebug() << FCFL << rx.isValid() << rx.pattern();

    qDebug() << FCFL << "________START FILE READ LOOP________";

    while (!sectionOver && (line = file.readLine()).size() > 0) {
        file.unsetError();
        ++lineNum;

        qDebug() << lineNum << line.trimmed();

        // skip lines until first line to capture
        if (lineNum < pathLineNum) continue;

        QString lineStr(QString::fromUtf8(line.trimmed()));

        if (lineNum > pathLineNum) {
            // exclude first line from regexp check

            int pos = rx.indexIn(lineStr);
            qDebug() << "...regexp result:" << pos << rx.captureCount();
            if (pos >= 0) {
                sectionOver = true; // last line in section

                if (rx.captureCount() <= 1) continue; // last line contains nothing to add to model

                lineStr = rx.cap(1); // just add captured part of last line
            }
        }

        if (!model()->insertRow(modelRow)) {
            qWarning() << "Failed to insert to row number" << modelRow << "to model at" << FCFL;
            return -2;
        }

        qDebug() << "...adding line";
        QModelIndex index = model()->index(modelRow, 0);

        if (!model()->setData(index,
                              lineStr)) {
            qWarning() << "Failed to set default role for model row" << modelRow;
        }

        if (!model()->setData(index,
                              QString("%1:%2").arg(pathInfo().canonicalFilePath(), QString::number(lineNum)),
                              ActualPathRole)) {
            qWarning() << "Failed to set ActualPathRole for model row" << modelRow;
        }
        ++modelRow;
    }

    qDebug() << FCFL << "________ END FILE READ LOOP ________";

    if (file.error() != QFile::NoError) {
        qDebug() << FCFL << "reading ended with readLine error" << file.errorString();
    }

    qDebug() << FCFL << model()->rowCount();
    return model()->rowCount();
}


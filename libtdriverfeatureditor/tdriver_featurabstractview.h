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

#ifndef TDRIVERFEATURABSTRACTVIEW_H
#define TDRIVERFEATURABSTRACTVIEW_H

#include "libtdriverfeatureditor_global.h"

#include <QWidget>

class QString;
class QListView;
class QComboBox;
class QAbstractItemModel;
class QToolBar;

//class QAction;

class LIBTDRIVERFEATUREDITORSHARED_EXPORT TDriverFeaturAbsractView : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString scanPattern READ scanPattern WRITE setScanPattern)
    Q_PROPERTY(QString path READ path WRITE setPath);
    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel);
    Q_PROPERTY(QComboBox* locationBox READ locationBox);

public:
    explicit TDriverFeaturAbsractView(const QString &title, QWidget *parent = 0);

    QString path() { return __path; }
    void setPath(const QString &path) { __path = path; }

    QString scanPattern() { return __scanPattern; }
    void setScanPattern(const QString &pattern) { __scanPattern = pattern; }

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model();
    void clearModel();

    QComboBox *locationBox() { return _locationBox; }


signals:

public slots:
    virtual void pullPath();

protected:
    int rescanPathWithPattern(); // <0 for error, >=0 for count
    QToolBar *_toolBar;
    QListView *_listView;
    QComboBox *_locationBox;
    //QAction *refreshAct;

private:
    QString __path;
    QString __scanPattern;
};

#endif // TDRIVERFEATURABSTRACTVIEW_H

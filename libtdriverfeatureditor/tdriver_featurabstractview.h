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
class QStyledItemDelegate;
class QComboBox;
class QAbstractItemModel;
class QToolBar;
class QFileInfo;
class QItemSelectionModel;
class QModelIndex;
class QAbstractItemView;
class QListView;

//class QAction;

class LIBTDRIVERFEATUREDITORSHARED_EXPORT TDriverFeaturAbstractView : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString scanPattern READ scanPattern WRITE setScanPattern)
    Q_PROPERTY(QString scanPattern2 READ scanPattern2 WRITE setScanPattern2)
    Q_PROPERTY(QString path READ path WRITE setPath);
    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel);
    Q_PROPERTY(QComboBox* locationBox READ locationBox);
    Q_PROPERTY(bool pendingScan READ pendingScan WRITE setPendingScan);
    Q_PROPERTY(int pathLineNumber READ pathLineNumber)
    //Q_PROPERTY(ScanType scanType READ scanType WRITE setScanType);


public:
    enum ScanType { NoScan, DirScan, FileScan, FileSectionScan };
    //Q_DECLARE_METATYPE(ScanType);
    enum DataRoles { ActualPathRole=Qt::UserRole+1, FileContentRole };

    explicit TDriverFeaturAbstractView(const QString &title, QWidget *parent = 0);
    ~TDriverFeaturAbstractView();

    const QFileInfo &pathInfo() { return *__pathInfo; }

    int pathLineNumber() { return __pathLine; }

    QString path();
    void setPath(const QString &path);

    QString scanPattern() { return __scanPattern; }
    void setScanPattern(const QString &pattern) { __scanPattern = pattern; }

    QString scanPattern2() { return __scanPattern2; }
    void setScanPattern2(const QString &pattern) { __scanPattern2 = pattern; }

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model();
    bool clearModel(int rows = 0);

    QComboBox *locationBox() { return __locationBox; }

    bool pendingScan() { return __pendingScan; }
    void setPendingScan(bool value=true) { __pendingScan = value; }

    ScanType scanType() {return __scanType; }
    void setScanType(ScanType value) { __scanType = value; }

    QItemSelectionModel *selectionModel();

    QAbstractItemView *view();


signals:
    void reScanned(const QString &path);

public slots:
    virtual void aFileChanged(const QString &path);
    virtual void resetPath(const QString &path);
    virtual void resetPathFromIndex(const QModelIndex &index); // null index will reset view
    virtual void resetPathFromBox();
    virtual void clearView();
    virtual int reScan(); // calls doXxxScan() and returns it's return value, 0 for NoScan, -2 for bad state


protected:
    void setLocationBox(const QString &text);

    virtual void showEvent(QShowEvent *);

    // scan method return <0 for error, >=0 for count
    virtual int doDirScan();
    virtual int doFileScan();
    virtual int doFileSectionScan();

    QToolBar *_toolBar;
    //QAction *refreshAct;
    QStyledItemDelegate *_styleDelegate;

private:
    QListView *__listView;
    QComboBox *__locationBox;
    QFileInfo *__pathInfo;
    int __pathLine;
    QString __scanPattern;
    QString __scanPattern2;

    bool __pendingScan;

    ScanType __scanType;

};

#endif // TDRIVERFEATURABSTRACTVIEW_H

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


#ifndef TDRIVER_TRANSLATIONDB_H
#define TDRIVER_TRANSLATIONDB_H


#include "libtdriverutil_global.h"

#include <QStringList>

typedef enum EDbType {
    Undefined = 0,
    MySQL = 1,
    SQLite = 2,
} EDbType;

class LIBTDRIVERUTILSHARED_EXPORT TDriverTranslationDb {
public:
    TDriverTranslationDb();
    ~TDriverTranslationDb();


    bool connectDB(QString host, QString name, QString table, QString user, QString password);
    void disconnectDB();
    EDbType getDbType();

    QStringList getAvailableLanguages();
    void setLanguageFilter(QStringList filter);
    QStringList getLanguageFilter();


    QList<QStringList> getTranslationsLike(QString text);
    QStringList findLNames(QString translation, QString language);
    QList<QStringList> findLNames(QString translation);
    QStringList findTranslations(QString lname, QString language);
    QList<QStringList> findTranslations(QString lname);

public:
    QStringList availableLanguages;

private:
    EDbType dbType;
    bool connected;
    QString locTable;
    QStringList languageFilter;

};

#endif // TDRIVER_TRANSLATIONDB_H

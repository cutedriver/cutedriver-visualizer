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


#include <QtSql>
#include "tdriver_translationdb.h"
#include <tdriver_debug_macros.h>



TDriverTranslationDb::TDriverTranslationDb()
{
    dbType = Undefined;
    connected = false;
    languageFilter << "English-GB";
}

TDriverTranslationDb::~TDriverTranslationDb()
{
    if (connected) disconnectDB();
}

bool TDriverTranslationDb::connectDB(QString aHost, QString aName, QString aTable, QString aUser, QString aPassword)
{
    QSqlDatabase db;
    dbType = MySQL;

    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName(aHost);
    db.setDatabaseName(aName);
    db.setUserName(aUser);
    db.setPassword(aPassword);
    locTable = aTable;
    connected =  db.open();
    if (!connected) {
        qDebug() << FFL << "connection error:" << db.lastError().text();
    }
    return connected;
}

void TDriverTranslationDb::disconnectDB()
{
    dbType = Undefined;
    QSqlDatabase db =  QSqlDatabase::database();
    db.close();
    connected = false;
}

EDbType TDriverTranslationDb::getDbType()
{
    return dbType;
}

QStringList TDriverTranslationDb::getAvailableLanguages()
{
    availableLanguages.clear();
    if (connected)
    {
        QSqlQuery query;
        query.exec("select COLUMN_NAME from `information_schema`.`COLUMNS` where TABLE_NAME = '" + locTable + "'");
        while (query.next())
        {
            QString col_name = query.value(0).toString();
            // Ignore ID, FNAME and LNAME columns
            if (col_name.toUpper() != "ID" && col_name.toUpper() != "FNAME" && col_name.toUpper() != "LNAME" )
                availableLanguages << col_name ;
        }
    }
    return availableLanguages;
}

void TDriverTranslationDb::setLanguageFilter(QStringList filter)
{
    languageFilter = filter;
}

QStringList TDriverTranslationDb::getLanguageFilter()
{
    return languageFilter;
}

QList<QStringList> TDriverTranslationDb::getTranslationsLike(QString searchText)
{
    QList<QStringList> results;
    if (connected)
    {
        QSqlQuery query;

        // If languageFilter not set
        if (languageFilter.isEmpty())
        {
//            // This query requires a FULLTEXT index to be set on all language columns on the DB
//            QString language_cols = "`" + availableLanguages.join("`, `") + "`";
//            query.exec("select " + language_cols + " from " + locTable + " where match (" + language_cols + ") against ('" + searchText + "')");
//            while (query.next())
//            {
//                // If using match against columns query,
//                // select from each result the column with the match
//                int index = 0;
//                while (query.value(index).isValid())
//                {
//                    if (query.value(index).toString().contains( searchText, Qt::CaseInsensitive))
//                    {
//                        results << query.value(index).toString();
//                        break;
//                    }
//                    index++;
//                }
//            }
        }

        // If languageFilter set
        else
        {
            //One query per language
            foreach (QString language, languageFilter)
            {
                query.exec("select lname, `" + language + "` from " + locTable + " where `" + language + "` like '%" + searchText + "%'");
                while (query.next())
                {
                    results << (QStringList() << query.value(0).toString()  << query.value(1).toString() << language );
                }
            }
        }
    }
    return results;
}

// Several LNames can have the same translation
// 1-Translation -> Many-Lnames (in the same language)
QStringList TDriverTranslationDb::findLNames(QString translation, QString language)
{
    QStringList results;
    if (connected)
    {
        QSqlQuery query;
        query.exec("select LNAME from " + locTable + " where " + language + " = '" + translation + "'");
        while (query.next())
        {
            results << query.value(0).toString();
        }
    }
    return results;
}

QList<QStringList> TDriverTranslationDb::findLNames(QString translation)
{
    QList<QStringList> results;

    if (languageFilter.isEmpty())
    {
        // TODO if languageFilter not set
    }
    else
    {
        foreach (QString language, languageFilter)
        {
            results << ( QStringList() << findLNames(translation, language) << language );
        }
    }
    return results;
}

// Same LNames (on diferent files (FNames)) can have different translations
// LNames are not unique ( Same LName for different language file (FName) )
// Many-LName -> Many-Translations (in the same language)
QStringList TDriverTranslationDb::findTranslations(QString lname, QString language)
{
    QStringList results;
    if (connected)
    {
        QSqlQuery query;
        query.exec("select " + language + " from " + locTable + " where LNAME = '" + lname + "'");
        while (query.next())
        {
            results << query.value(0).toString();
        }
    }
    return results ;
}

QList<QStringList> TDriverTranslationDb::findTranslations(QString lname)
{
    QList<QStringList> results;

    if (languageFilter.isEmpty())
    {
        // TODO if languageFilter not set
    }
    else
    {
        foreach (QString language, languageFilter)
        {
            results << (QStringList() << findTranslations( lname, language) << language);
        }
    }

    return results;
}


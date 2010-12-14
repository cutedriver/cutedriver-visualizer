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



#include <QtPlugin>
#include <QDebug>
#include <QHash>

#include <QVariant>

//#include <QDomElement>
//#include <QDomDocument>

#include <visualizeraccessor.h>
#include <tasqtdatamodel.h>

#include <tdriver_tabbededitor.h>
//
//#include <dispatcher.h>
//#include <includes.h>

Q_EXPORT_PLUGIN2(visualizeraccessor, VisualizerAccessor)

// fixture version information
static QString FIXTURE_NAME = "VisualizerAccessor";
static QString FIXTURE_VERSION = "0.1";
static QString FIXTURE_DESCRIPTION = "API for accessing applications internal Qt methods";

// hash keys
static QString CLASS_NAME = "class";
static QString METHOD_NAME = "method";
static QString METHOD_ARGS = "args";

// Constructor
VisualizerAccessor::VisualizerAccessor( QObject *parent ) : QObject( parent )
{

}


bool VisualizerAccessor::execute(void * objectInstance, QString commandName, QHash<QString, QString> parameters, QString & resultString)
{
    qDebug() << "VisualizerAccessor::execute";
    qDebug() << "command:" << commandName;
    qDebug() << "parameters" << parameters;

    bool result = false;
    resultString = commandName;

    static QStringList valid_commands(QStringList() << "ping" << "editor_save" << "editor_load");

    if (!valid_commands.contains(commandName)) {
        resultString += " error: invalid command";
    }

    // first commands that do not need editor instance
    else if (commandName == "ping") {
        if (parameters.contains("data")) resultString = parameters["data"];
        else resultString = "pong";
        result = true;
    }

    // then commands that need editor instance
    else {
        // TODO: add code to check that objectInstance really is a QObject pointer
        TDriverTabbedEditor *tabs = qobject_cast<TDriverTabbedEditor*>(
                    reinterpret_cast<QObject*>(objectInstance));
        QString className = (tabs) ? tabs->metaObject()->className() : "<invalid>";

        if (!tabs) {
            resultString = (commandName + " error: invalid object @0x%1 class name '%2'")
                    .arg(reinterpret_cast<quintptr>(objectInstance), 8, 16, QChar('0'))
                    .arg(className);
        }

        else if (commandName == "editor_save") {
            if (parameters.contains("filename")) {
                int index = QVariant(parameters.value("tabindex", "-1")).toInt(&result);
                if (result) {
                    result = tabs->saveFile(parameters["filename"], index, true);
                    resultString += ": saveFile called";
                }
                else {
                    resultString += " error: invalid tab index";
                }
            }
            else {
                resultString += " error: missing command parameter: filename";
            }
        }

        else if (commandName == "editor_load") {
            if (parameters.contains("filename")) {
                result = tabs->loadFile(parameters["filename"], QVariant(parameters.value("istemplate", "false")).toBool());
                resultString += ": loadFile called";
            }
            else
                resultString += " error: missing command parameter: filename";
        }
    }

    return result;

}

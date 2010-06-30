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
    QString className;
    QObject *tmpQObject = reinterpret_cast<QObject*>(objectInstance);
    if (tmpQObject) className = tmpQObject->metaObject()->className();
    //    resultString.append(" "+className);
    TDriverTabbedEditor *tabs = qobject_cast<TDriverTabbedEditor*>(tmpQObject);
    //    resultString = (resultString + " & %1 %2 %3").
    //            arg(reinterpret_cast<uint>(objectInstance), 8, 16, QChar('0')).
    //            arg(reinterpret_cast<uint>(tmpQObject), 8, 16, QChar('0')).
    //            arg(reinterpret_cast<uint>(tabs), 8, 16, QChar('0'));

    if (commandName == "editor_save") {
        if (tabs) {
            if (parameters.contains("filename")) {
                int index = QVariant(parameters.value("tabindex", "-1")).toInt(&result);
                if (result) {
                    result = reinterpret_cast<TDriverTabbedEditor*>(objectInstance)->saveFile(
                            parameters["filename"], index);
                    resultString += ": saveFile called";
                }
                else
                    resultString += " error: invalid tab index";
            }
            else
                resultString += " error: missing command parameter: filename";
        }
        else {
            resultString = (commandName + " error: invalid object @0x%1 class name '%2'").
                           arg(reinterpret_cast<uint>(tmpQObject), 8, 16, QChar('0')).
                           arg(className);
        }
    }
    else if (commandName == "editor_load") {
        if (tabs) {
            if (parameters.contains("filename")) {
                result = reinterpret_cast<TDriverTabbedEditor*>(objectInstance)->loadFile(
                        parameters["filename"], QVariant(parameters.value("istemplate", false)).toBool());
                resultString += ": loadFile called";
            }
            else
                resultString += " error: missing command parameter: filename";
        }
        else {
            resultString = (commandName + " error: invalid object @0x%1 class name '%2'").
                           arg(reinterpret_cast<uint>(tmpQObject), 8, 16, QChar('0')).
                           arg(className);
        }
    }
    else if (commandName == "ping") {
        if (parameters.contains("data")) resultString = parameters["data"];
        else resultString = "pong";
        result = true;
    }
    else {
        resultString += " error: invalid command";
    }

    return result;

}

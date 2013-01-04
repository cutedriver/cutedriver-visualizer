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



#ifndef VISUALIZERACCESSOR_H
#define VISUALIZERACCESSOR_H

#include <QObject>
#include <QHash>
#include <QVariant>
#include <tasfixtureplugininterface.h>
#include <tasqtdatamodel.h>

class VisualizerAccessor : public QObject, public TasFixturePluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.nokia.testability.VisualizerAccessor" FILE "visualizeraccessor.json")
    Q_INTERFACES(TasFixturePluginInterface)

public:
    VisualizerAccessor(QObject* parent=0);

    bool execute(void* objectInstance, QString commandName, QHash<QString, QString> parameters, QString & resultString);


private:
    QObject* castToObject(void* objectInstance, QString ptrType);


};

#endif


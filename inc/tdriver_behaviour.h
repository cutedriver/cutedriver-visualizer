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
 
 
#include <QStringList>
#include <QMap>

class Behaviour {

public:

    Behaviour();    // constructor
    ~Behaviour();    // destructor

    void addSutType( QString sutType );
    QStringList getSutTypes();

    void addControlMethod( QString controlType );
    QStringList getControlMethods();

    void addMethod(QString methodName, QMap<QString, QString> methodHash );
    QMap<QString, QString> getMethod( QString methodName );

    QStringList getMethodsList();

private:

    QStringList sutTypes;
    QStringList controlMethods;
    QMap<QString, QMap<QString, QString> > methods;

};


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
 
 

#include "tdriver_behaviour.h"

// constructor
Behaviour::Behaviour() {

    sutTypes.clear();
    controlMethods.clear();
    methods.clear();

}

// destructor
Behaviour::~Behaviour() {

    sutTypes.clear();
    controlMethods.clear();
    methods.clear();

}

void Behaviour::addSutType( QString sutType ) {

    if ( !sutTypes.contains( sutType ) ) { sutTypes << sutType; } 

}


QStringList Behaviour::getSutTypes() {

    return sutTypes;

}

void Behaviour::addControlMethod( QString controlMethod ) {

    if ( !controlMethods.contains( controlMethod ) ) { controlMethods << controlMethod; } 
    
}

QStringList Behaviour::getControlMethods() {

    return controlMethods;

}

void Behaviour::addMethod(QString methodName, QMap<QString, QString> methodHash ) {

    if ( !methods.contains( methodName ) ) { methods.insert( methodName, methodHash ); }

}

QMap<QString, QString> Behaviour::getMethod( QString methodName ) {

    QMap<QString, QString> result;

    if ( methods.contains( methodName ) ) { result = methods.value( methodName ); }

    return result;

}

QStringList Behaviour::getMethodsList() {

    return methods.keys();

}



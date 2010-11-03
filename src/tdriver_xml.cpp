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



#include "tdriver_main_window.h"



bool MainWindow::updateBehaviourXml()
{

    if (objectTree->invisibleRootItem()->childCount() <= 0) return false;

    bool result = false;

    QStringList objectTypes;

    QTreeWidgetItem *root = objectTree->invisibleRootItem();
    QTreeWidgetItem *node = root;

    while ( (node = findDialogSubtreeNext(node, root)) ) {
        Q_ASSERT(objectTreeData.contains(( TestObjectKey )node ));
        QHash<QString, QString> objectTreeItemData = objectTreeData.value( ( TestObjectKey )node);
        QString objectType = objectTreeItemData.value( "type" );

        if ( !objectTypes.contains( objectType ) && !behavioursMap.contains( objectType ) ) {
            objectTypes << objectType;
        }
    }


    QString objectTypesString;

    // build a string of object types
    for ( int index = 0; index < objectTypes.size(); index++ ) {
        if ( !objectTypesString.isEmpty() ) {
            objectTypesString += ",";
        }
        objectTypesString += "'" + objectTypes.at( index ) + "'";
    }

    if ( executeTDriverCommand( commandBehavioursXml,
                          activeDevice.value( "name" ) + " get_behaviours " + objectTypesString ) &&
         objectTypes.size() > 0 ) {
        parseXml( outputPath + "/visualizer_behaviours_" + activeDevice.value( "name" ) + ".xml" , behaviorDomDocument );
        buildBehavioursMap();
    }

    propertyTabLastTimeUpdated.insert( "methods", 0 );
    return result;
}

void MainWindow::buildBehavioursMap() {

    // qDebug() << "buildBehavioursMap()";

    // behavioursMap.clear();

    QDomNodeList nodeList = behaviorDomDocument.documentElement().elementsByTagName( "behaviour" );

    for ( int behaviourIndex = 0; behaviourIndex < nodeList.size(); behaviourIndex++ ) {

        QDomNode node = nodeList.item( behaviourIndex );

        if ( node.isElement() ) {

            /*

<?xml version="1.0"?>
<behaviours>
<behaviour object_type="sut">
<object_method name="kill_started_processes">
        <description>Kills all of the application processed started through the server.</description>
        <example>kill_started_processes</example>
      </object_method><object_method name="list_apps">
        <description>Lists all applications known to server running on SUT.</description>
        <example>list_apps</example>


*/

            // retrieve element from node
            QDomElement domElement = node.toElement();

            // retrieve target object name
            QString targetObject       = domElement.attribute( "object_type" );

            // retrieve object methods
            QDomNodeList methodsNodeList = domElement.elementsByTagName( "object_method" );

            //qDebug() << "target object: " << targetObject;

            Behaviour behaviour;

            // retrieve method from behaviours list if one already exists
            if ( behavioursMap.contains( targetObject ) ) { behaviour = behavioursMap.value( targetObject ); }

            for ( int methodIndex = 0; methodIndex < methodsNodeList.size(); methodIndex++ ) {

                QDomElement methodElement = methodsNodeList.item( methodIndex ).toElement();

                QString methodName = methodElement.attribute( "name" );

/*
                QString methodDescription("");

                QXmlStreamReader xmlReader(methodElement.elementsByTagName( "description" ).item( 0 ).toElement().text());
                while (!xmlReader.atEnd()) {
                    if ( xmlReader.readNext() == QXmlStreamReader::Characters ) {
                        methodDescription += xmlReader.text();
                    }
                }

                QString methodExample("");

                xmlReader.clear();
                xmlReader.addData(methodElement.elementsByTagName( "example" ).item( 0 ).toElement().text());
                while (!xmlReader.atEnd()) {
                    if ( xmlReader.readNext() == QXmlStreamReader::Characters ) {
                        methodExample += xmlReader.text();
                    }
                }

                //qDebug() << methodName << methodDescription << methodExample;

                QMap<QString, QString> methodDetails;

                methodDetails.insert( "description", methodDescription );
                methodDetails.insert( "example", methodExample );
*/

                //QString methodDescription = methodElement.elementsByTagName( "description" ).item( 0 ).toElement().text();

                //QString methodExample = methodElement.elementsByTagName( "example" ).item( 0 ).toElement().text();

                //qDebug() << methodName << methodDescription << methodExample;

                QMap<QString, QString> methodDetails;

                methodDetails.insert( "description", (QString) methodElement.elementsByTagName( "description" ).item( 0 ).toElement().text() );

                methodDetails.insert( "example", (QString) methodElement.elementsByTagName( "example" ).item( 0 ).toElement().text() );

                behaviour.addMethod( methodName, methodDetails );

            }

            behavioursMap.insert( targetObject, behaviour );

        }


    }

}

void MainWindow::updateApplicationsList() {

    // qDebug() << "MainWindow::updateApplicationsList()";

    bool found = false;

    appsMenu->clear();

    QHashIterator<QString, QString> iterator( applicationsHash );

    applicationsProcessIdMap.clear();

    int count = 0;

    // foreground application selection
    QAction * appAction = new QAction( appsMenu );
    appAction->setObjectName("main foregroundapp");
    appAction->setText( "Foreground application" );
    appAction->setCheckable( true );

    appAction->setShortcut( QKeySequence( "ALT+" + QString::number( 0 ) ) );
    applicationsProcessIdMap.insert( (ProcessKey)(appAction), "0" );
    connect( appAction, SIGNAL( triggered() ), this, SLOT( appSelected() ) );
    appsMenu->addAction( appAction );

    if ( foregroundApplication ) {
        appAction->setChecked( true );
    }

    // add separator to menu
    appsMenu->addSeparator();

    while ( iterator.hasNext() ) {

        iterator.next();

        QAction * appAction = new QAction( appsMenu );
        appAction->setObjectName("main app "+iterator.value());
        appAction->setText( QString( iterator.value() ) + "  (" + QString( iterator.key() ) + ")" );
        appAction->setCheckable( true );

        appAction->setShortcut( QKeySequence( "ALT+" + QString::number( count + 1 ) ) );

        if ( iterator.key() == currentApplication.value( "id" ) && !foregroundApplication ) {

            appAction->setChecked( true );

            currentApplication.clear();
            currentApplication.insert( "id", iterator.key() );
            currentApplication.insert( "name", iterator.value() );

            found = true;

        }

        applicationsProcessIdMap.insert( (ProcessKey)(appAction), iterator.key() );

        connect( appAction, SIGNAL( triggered() ), this, SLOT( appSelected() ) );
        appsMenu->addAction( appAction );
        count++;

    }

    if ( !found ) {

        // no current application found from list - clear it...
        currentApplication.clear();

    }

}

void MainWindow::resetApplicationsList() {

    applicationsHash.clear();
    appsMenu->clear();

}

void MainWindow::parseApiMethodsXml( QString filename ) {

    QDomDocument apiDocument;

    if ( parseXml( filename, apiDocument ) ) {

        QMap<QString, QHash<QString, QString> > methodMap;

        QDomElement infoElement = apiDocument.documentElement().elementsByTagName( "tasInfo" ).item( 0 ).toElement();

        QString className = infoElement.attribute("name");

        QDomNodeList methodNodes = infoElement.elementsByTagName( "object" );

        // collect method names, arguments and return value types
        for ( int methodIndex = 0; methodIndex < methodNodes.size(); methodIndex++ ) {

            QHash<QString, QString> attributes;

            /*
            <tasMessage dateTime="2009.09.03 13:39:48.503" version="0.4.3-1" >
              <tasInfo id="0" name="Button" type="QtMethods" >
                  <object id="0" name="attributeName" parent="" type="QtMethod" >
                  <attributes>
                      <attribute name="returnValueType" type="" >
                      <value>
                        const QString
                      </value>
                      </attribute>
                      <attribute name="arguments" type="" >
                      <value>
                      </value>
                      </attribute>
                  </attributes>
                  </object>
              </tasInfo>
            </tasMessage>
            */

            QDomElement methodElement = methodNodes.item( methodIndex ).toElement();
            QDomNodeList attributesNodeList = methodElement.firstChildElement( "attributes" ).elementsByTagName( "attribute" );

            for ( int attributeIndex = 0; attributeIndex < attributesNodeList.size(); attributeIndex++ ) {

                QDomElement attributeElement = attributesNodeList.item( attributeIndex ).toElement();
                attributes.insert( attributeElement.attribute( "name" ), attributeElement.firstChildElement( "value" ).text() );

            }

            methodMap.insert( methodElement.attribute( "name" ), attributes );

        }

        if ( !apiMethodsMap.contains( className ) ) { apiMethodsMap.insert( className, methodMap ); }

    }

}
QStringList MainWindow::parseSignalsXml( QString filename ) {

    QStringList signalList;
    QDomDocument apiDocument;

    QFile f(filename);
    if ( f.exists() && parseXml( filename, apiDocument ) ) {


        QDomElement infoElement = apiDocument.documentElement().elementsByTagName( "tasInfo" ).item( 0 ).toElement();
        QDomNodeList signalNodes = infoElement.elementsByTagName( "object" );

        // collect signal names
        for ( int signalIndex = 0; signalIndex < signalNodes.size(); signalIndex++ ) {

            /*
                            <tasMessage dateTime="2010.05.19 18:22:07.000" version="0.7.2" >
                              <tasInfo id="1" name="QtSignals" type="QtSignals" >
                                <object id="0" name="destroyed(QObject*)" type="QtSignal" ></object>
                                <object id="1" name="destroyed()" type="QtSignal" ></object>
                                <object id="4" name="customContextMenuRequested(QPoint)" type="QtSignal" ></object>
                                <object id="27" name="pressed()" type="QtSignal" ></object>
                                <object id="28" name="released()" type="QtSignal" ></object>
                                <object id="29" name="clicked(bool)" type="QtSignal" ></object>
                                <object id="30" name="clicked()" type="QtSignal" ></object>
                                <object id="31" name="toggled(bool)" type="QtSignal" ></object>
                                <object id="39" name="triggered(QAction*)" type="QtSignal" ></object>
                              </tasInfo>
                            </tasMessage>
                        */

            signalList.append(signalNodes.item(signalIndex ).toElement().attribute( "name" ));

        }
    }
    return signalList;
}

void MainWindow::parseApplicationsXml( QString filename ) {

    QDomNode nodeInfo;
    QDomNode nodeApplications;

    QDomNodeList appObjects;
    QDomNodeList applications;

    QDomDocument appDocument;

    resetApplicationsList();

    if ( parseXml( filename, appDocument ) ) {

        QDomElement root = appDocument.documentElement();

        nodeInfo = root.firstChild();

        if ( !nodeInfo.isNull() ) {

            //qDebug("xml2View: node name:%s", node.nodeName().toLatin1().data());
            if ( nodeInfo.isElement() && nodeInfo.nodeName() == "tasInfo" ) {

                nodeApplications = nodeInfo.firstChild();

                if ( nodeApplications.isElement() && nodeApplications.toElement().attribute( "type" ) == "applications" )    {

                    appObjects = nodeApplications.toElement().elementsByTagName( "objects" );

                    if ( appObjects.size() > 0 ) {

                        applications = appObjects.item( 0 ).toElement().elementsByTagName( "object" );

                        for (int i = 0; i < applications.size(); i++) {

                            QDomNode application = applications.item( i );

                            if ( application.isElement() ) {
                                QDomElement appElement = application.toElement();
                                applicationsHash.insert( appElement.attribute( QString( "id" ) ), appElement.attribute( QString( "name" ) ) );
                            }
                        }

                    }
                }
            }
        }

    }

    updateApplicationsList();

    //if ( !applicationsHash.contains( strActiveAppName ) ) {    strActiveAppName = ""; }
    if ( !applicationsHash.contains( currentApplication.value( "id" ) ) ) { currentApplication.clear(); }

    // Disable menu if it has no applications
    appsMenu->setEnabled(!applicationsHash.empty());

    // Disable record menu if active device type is S60
    if ( !activeDevice.value( "type" ).contains( "s60", Qt::CaseInsensitive ) ) {
        recordMenu->setEnabled( !applicationsHash.empty() );
    }

}


bool MainWindow::parseXml( QString fileName, QDomDocument & resultDocument ) {

    // temporary xml dom document
    QDomDocument tempDomDocument;
    bool result = false;

    // read xml file
    QFile xmlFile( fileName );

    if ( !xmlFile.exists() ) {

        QMessageBox::critical(
                0,
                tr( "Error" ),
                tr( "File not found:\n\n  %1\n" ).arg( fileName )
                );

    } else {

        if ( !xmlFile.open( QIODevice::ReadOnly ) ) {

            QMessageBox::critical(
                    0,
                    tr( "Error" ),
                    tr( "Cannot open XML file %1" ).arg( fileName )
                    );

        } else {

            result = tempDomDocument.setContent( &xmlFile );

            if ( !result )  {

                QMessageBox::critical(
                        0,
                        tr( "Error" ),
                        tr( "Cannot parse xml file %1" ).arg( fileName )
                        );

            } else {

                // return parsed xml dom as result
                resultDocument = tempDomDocument;

            }

            xmlFile.close();

        }

    }

    return result;

}


bool MainWindow::getXmlParameters( QString filename )
{
    QDomDocument tmpDomTree;
    QDomNode node;
    QMap<QString, QHash<QString, QString> > tmpDeviceList;
    QMap<QString, QString> tmpXmlParameters;
    bool ok = false;

    if ( QFile::exists( filename ) ) {

        // read parameters xml file
        if ( parseXml( filename, tmpDomTree ) ) {
            QDomElement root = tmpDomTree.documentElement();
            node = root.firstChild();

            while ( !node.isNull() )  {

                if ( node.isElement() ) {
                    if (node.nodeName() == "sut" ) {
                        QString id = node.toElement().attribute("id");
                        QHash<QString, QString> sut;
                        sut.insert( "name", id);
                        sut.insert( "type", getDeviceType( id ) );
                        sut.insert( "default_timeout", getDeviceParameter( id, "default_timeout" ));
                        tmpDeviceList.insert(id, sut);
                    }

                    else if (node.nodeName() == "parameter" ) {
                        // top level parameters
                        QString name = node.toElement().attribute("name");
                        QString value = node.toElement().attribute("value");
                        tmpXmlParameters.insert(name, value);
                    }
                    //else qDebug() << FCFL << "Skipping element node" << node.nodeName;
                }

                node = node.nextSibling();
            }

            ok = true;
        }
    }

    if (ok) {
        updateDevicesList(tmpDeviceList);
        tdriverXmlParameters = tmpXmlParameters;
    }
    else {
        QMessageBox::warning(
                0,
                tr("Loading XML Failed"),
                tr("Failed to load and parse file:\n\n  %1").arg(filename));
    }

    return ok;
}


void MainWindow::parseBehavioursXml( QString filename ) {

    parseXml( filename, behaviorDomDocument );

}






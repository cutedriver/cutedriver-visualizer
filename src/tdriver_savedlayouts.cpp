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

#include <QtGui>

#include <tdriver_debug_macros.h>


void MainWindow::settingsSaveLayouts()
{
    QSettings settings;

    settings.beginWriteArray("view/savedlayouts");
    for(int ii = 0; ii < SAVEDLAYOUTCOUNT; ++ii) {
        settings.setArrayIndex(ii);
        settings.setValue("name", savedLayouts[ii].name);
        // TODO: when/if name changes supported, then
        //       update layout save/restore action texts to match names
        settings.setValue("geometry", savedLayouts[ii].geometry);
        settings.setValue("state", savedLayouts[ii].state);
        settings.setValue("windowState", (int)savedLayouts[ii].windowState);
        qDebug() << FCFL << ii << "wrote state" << savedLayouts[ii].windowState;
    }
    settings.endArray();

    settings.setValue("view/startup_geometry", saveGeometry());
    settings.setValue("view/startup_state", saveState());

}

void MainWindow::settingsLoadLayouts()
{
    QSettings settings;

    Q_ASSERT(saveLayoutActions.count() == SAVEDLAYOUTCOUNT);
    Q_ASSERT(restoreLayoutActions.count() == SAVEDLAYOUTCOUNT);

    settings.beginReadArray("view/savedlayouts");
    for(int ii = 0; ii < SAVEDLAYOUTCOUNT; ++ii) {
        settings.setArrayIndex(ii);
        savedLayouts[ii].name = settings.value("name", tr("Layout")+QString::number(ii+1)).toString();
        savedLayouts[ii].geometry = settings.value("geometry").toByteArray();
        savedLayouts[ii].state = settings.value("state").toByteArray();
        savedLayouts[ii].windowState = Qt::WindowStates(settings.value("windowState").toInt());
        qDebug() << FCFL << ii << "loaded state" << savedLayouts[ii].windowState;
    }
    settings.endArray();
}


void MainWindow::saveALayout(QAction *layoutSaveAction)
{
    if (!layoutSaveAction) {
        layoutSaveAction = qobject_cast<QAction*>(sender());
        if (!layoutSaveAction) return;
    }

    int ind = saveLayoutActions.indexOf(layoutSaveAction);
    if (ind >= 0 && ind < SAVEDLAYOUTCOUNT) {
        qDebug() << FCFL << ind << layoutSaveAction->text();
        savedLayouts[ind].state = saveState();
        savedLayouts[ind].geometry = saveGeometry();
        savedLayouts[ind].windowState = windowState();
        qDebug() << FCFL << ind << "saved state" << savedLayouts[ind].windowState;
    }
    // else not action for this method to handle, ignore
}


void MainWindow::restoreALayout(QAction *layoutSetAction)
{
    if (!layoutSetAction) {
        layoutSetAction = qobject_cast<QAction*>(sender());
        if (!layoutSetAction) return;
    }

    int ind = restoreLayoutActions.indexOf(layoutSetAction);

    if (ind >= 0 && ind < SAVEDLAYOUTCOUNT) {
        qDebug() << FCFL << ind << layoutSetAction->text();

        if (savedLayouts[ind].isValid()) {
            showNormal(); // needed, or restoreGeometry will not restore fullscreen state properly
            if (!restoreGeometry(savedLayouts[ind].geometry)) {
                qDebug() << FCFL << "restoreGeometry fail";
            }

            if(!restoreState(savedLayouts[ind].state)) {
                qDebug() << FCFL << "restoreState fail";
            }

            qDebug() << FCFL << ind << "restroring state" << savedLayouts[ind].windowState;

#if 0
            //setWindowState(savedLayouts[ind].windowState);
            if (savedLayouts[ind].windowState & Qt::WindowFullScreen) {
                qDebug() << FCFL << "showFullscreen";
                showFullScreen();
            }
            else if (savedLayouts[ind].windowState & Qt::WindowMaximized) {
                qDebug() << FCFL << "showMaximized";
                showMaximized();
            }
            else {
                qDebug() << FCFL << "showNormal" << sizeof( QByteArray);
                showNormal();
            }
#else
            qDebug() << FCFL << "stored window state" << QString::number(savedLayouts[ind].windowState, 16)
                     << " vs current restored window state" << QString::number(windowState(), 16);
#endif
        }
        else {
            QMessageBox::StandardButton button = QMessageBox::question(
                        this,
                        tr("Save layout?"),
                        tr("%1 not set, save?").arg(layoutSetAction->text()),
                        QMessageBox::Yes|QMessageBox::No,
                        QMessageBox::Yes);
            if (button == QMessageBox::Yes) {
                saveALayout(saveLayoutActions.value(ind));
            }
        }
    }
    // else not action for this method to handle, ignore

}


void MainWindow::setStartupLayout()
{
    QSettings settings;
    QByteArray geom = settings.value("view/startup_geometry").toByteArray();
    QByteArray state = settings.value("view/startup_state").toByteArray();
    if (!state.isEmpty() && !geom.isEmpty()) {
        bool gOk = restoreGeometry(geom);
        bool sOk = restoreState(state);
        qDebug() << FCFL << "Restored startup layout:" << gOk << sOk;
    }
}

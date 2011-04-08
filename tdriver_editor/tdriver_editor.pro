# ###########################################################################
# #
# # Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
# # All rights reserved.
# # Contact: Nokia Corporation (testabilitydriver@nokia.com)
# #
# # This file is part of Testability Driver.
# #
# # If you have questions regarding the use of this file, please contact
# # Nokia at testabilitydriver@nokia.com .
# #
# # This library is free software; you can redistribute it and/or
# # modify it under the terms of the GNU Lesser General Public
# # License version 2.1 as published by the Free Software Foundation
# # and appearing in the file LICENSE.LGPL included in the packaging
# # of this file.
# #
# ###########################################################################
include (../visualizer.pri)
TEMPLATE = app
TARGET = tdriver_visualizer
DEPENDPATH += .. \
    ../inc
INCLUDEPATH += .. \
    ../inc
RC_FILE = ../vis.rc
CONFIG += link_prl

# For libutil
INCLUDEPATH += $$UTILLIBDIR
#LIBS += -L$$UTILLIBDIR -l$$UTIL_LIB
LIBS += -l$$UTIL_LIB

!CONFIG(no_sql) {
    QT += sql
}

# For libtdrivereditor:
INCLUDEPATH += $$EDITORLIBDIR
#LIBS += -L$$EDITORLIBDIR -l$$EDITOR_LIB
LIBS += -l$$EDITOR_LIB
QT += network

# Input
HEADERS += ../inc/tdriver_main_types.h
HEADERS += ../inc/tdriver_behaviour.h
HEADERS += ../inc/tdriver_image_view.h
HEADERS += ../inc/tdriver_main_window.h
HEADERS += ../inc/tdriver_recorder.h
SOURCES += ../src/tdriver_libeditor_ui.cpp
SOURCES += ../src/tdriver_editor.cpp
SOURCES += ../src/tdriver_main_window.cpp
SOURCES += ../src/tdriver_image_view.cpp
SOURCES += ../src/tdriver_recorder.cpp
SOURCES += ../src/tdriver_behaviours.cpp
SOURCES += ../src/tdriver_image_widget.cpp
SOURCES += ../src/tdriver_keyboard_commands_widget.cpp
SOURCES += ../src/tdriver_menu.cpp
SOURCES += ../src/tdriver_object_tree.cpp
SOURCES += ../src/tdriver_properties_table.cpp
SOURCES += ../src/tdriver_show_xml.cpp
SOURCES += ../src/tdriver_ui.cpp
SOURCES += ../src/tdriver_xml.cpp
SOURCES += ../src/tdriver_find_dialog.cpp
SOURCES += ../src/tdriver_startapp_dialog.cpp
QT += xml

# install
unix: {
    target.path = /opt/tdriver_visualizer
    documentation.path = /opt/tdriver_visualizer/help
    documentation.files = ../doc/help/qdoc-temp/*

    # tdriver_editor.files = debian/etc/tdriver/visualizer/tdriver_visualizer.sh
    tdriver_editor_complete.files += /usr/lib/libaudio.so.2
    tdriver_editor_complete.files += /usr/lib/libaudio.so.2.4
    tdriver_editor_complete.files += $$QMAKE_LIBDIR_QT/libQtCore.so.4
    tdriver_editor_complete.files += $$QMAKE_LIBDIR_QT/libQtGui.so.4
    tdriver_editor_complete.files += $$QMAKE_LIBDIR_QT/libQtSql.so.4
    tdriver_editor_complete.files += $$QMAKE_LIBDIR_QT/libQtXml.so.4
    tdriver_editor_complete.files += $$QMAKE_LIBDIR_QT/libQtNetwork.so.4
    tdriver_editor_complete.files += $$DESTDIR/libtdrivereditor.so.1
    tdriver_editor_complete.files += $$DESTDIR/libtdriverutil.so.1
    tdriver_editor_complete.files += $$DESTDIR/libcommon.so.1
    tdriver_editor_complete.files += $$EDITOR_PROTO_DIR/editor_proto
    tdriver_editor_complete.path = /opt/tdriver_visualizer
    OBJECTS_DIR = ../build/tdriver_editor
    MOC_DIR = ../build/tdriver_editor

    # Standalone version, no dependencies embedded
    tdriver_editor_standalone.files = ../bin/tdriver_visualizer
    tdriver_editor_standalone.path = /usr/bin

    tdriver_editor_standalone_share.files = $$EDITORLIBDIR/templates/* $$EDITORLIBDIR/completions/*
    tdriver_editor_standalone_share.path = /etc/tdriver/visualizer

# Pending new documentation
# TODO add just a link to the online doc
#    documentation_standalone.path = /usr/share/doc/visualizer/help
#    documentation_standalone.files = ../doc/help/qdoc-temp/*

    INSTALLS += tdriver_editor_standalone \
        tdriver_editor_standalone_share
#        documentation_standalone
}

win32: {
    MY_QTDIR = $$dirname(QMAKE_QMAKE)
    target.path = C:/tdriver/visualizer
    tdriver_editor_complete.path = C:/tdriver/visualizer
    tdriver_editor_complete.files += $$MY_QTDIR/QtXml4.dll
    tdriver_editor_complete.files += $$MY_QTDIR/QtCore4.dll
    tdriver_editor_complete.files += $$MY_QTDIR/QtGui4.dll
    tdriver_editor_complete.files += $$MY_QTDIR/QtNetwork4.dll

    documentation.path = C:/tdriver/visualizer/help
    documentation.files = ../doc/help/qdoc-temp/*
}

# message($${tdriver_editor_complete.files})

INSTALLS += tdriver_editor_complete
INSTALLS += documentation
INSTALLS += target

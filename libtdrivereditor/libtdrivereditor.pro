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
# -------------------------------------------------
# Project created by QtCreator 2010-02-19T10:17:24
# -------------------------------------------------
include (../visualizer.pri)
TARGET = tdrivereditor
DESTDIR = ./
win32:DLLDESTDIR = ../bin
TEMPLATE = lib
CONFIG += shared
CONFIG += create_prl
QT += network

//DEFINES += QT_NO_CAST_FROM_ASCII

# For libutil
CONFIG += link_prl
INCLUDEPATH += $$UTILLIBDIR
LIBS += -L$$UTILLIBDIR \
    -l$$UTIL_LIB
SOURCES += tdriver_tabbededitor.cpp \
    tdriver_runconsole.cpp \
    tdriver_rubyhighlighter.cpp \
    tdriver_highlighter.cpp \
    tdriver_debugconsole.cpp \
    tdriver_consoletextedit.cpp \
    tdriver_codetextedit.cpp \
    tdriver_editor_common.cpp \
    tdriver_rubyinteract.cpp \
    tdriver_editbar.cpp \
    tdriver_combolineedit.cpp
HEADERS += tdriver_tabbededitor.h \
    tdriver_runconsole.h \
    tdriver_rubyhighlighter.h \
    tdriver_highlighter.h \
    tdriver_debugconsole.h \
    tdriver_consoletextedit.h \
    tdriver_codetextedit.h \
    tdriver_editor_common.h \
    tdriver_rubyinteract.h \
    tdriver_editbar.h \
    tdriver_combolineedit.h

# install
unix:!symbian {
    target.path = /usr/lib
    editor_completions.path = /etc/tdriver/visualizer/completions
    editor_templates.path = /etc/tdriver/visualizer/templates
}
win32: {
    target.path = C:/tdriver/visualizer
    editor_completions.path = C:/tdriver/visualizer/completions
    editor_templates.path = C:/tdriver/visualizer/templates
}
editor_completions.files = completions/*
editor_templates.files = templates/*
INSTALLS += target
INSTALLS += editor_completions \
    editor_templates

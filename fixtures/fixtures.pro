############################################################################
##
## Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
## All rights reserved.
## Contact: Nokia Corporation (testabilitydriver@nokia.com)
##
## This file is part of Testability Driver.
##
## If you have questions regarding the use of this file, please contact
## Nokia at testabilitydriver@nokia.com .
##
## This library is free software; you can redistribute it and/or
## modify it under the terms of the GNU Lesser General Public
## License version 2.1 as published by the Free Software Foundation
## and appearing in the file LICENSE.LGPL included in the packaging
## of this file.
##
############################################################################

include (../visualizer.pri)

TEMPLATE = lib
TARGET = visualizeraccessor
CONFIG += plugin shared

# For libtdrivereditor:
CONFIG += link_prl
INCLUDEPATH += $$EDITORLIBDIR
#LIBS += -L$$EDITORLIBDIR -l$$EDITOR_LIB
LIBS += -l$$EDITOR_LIB


unix: {
TAS_TARGET_BIN=/usr/bin
TAS_TARGET_LIB=/usr/lib
TAS_TARGET_PLUGIN=$$[QT_INSTALL_PLUGINS]/
}
win32: {
QTTAS_DIRECTORY=C:/tdriver/qttas
TAS_TARGET_BIN=C:/qttas/bin
TAS_TARGET_LIB=C:/qttas/lib
TAS_TARGET_PLUGIN=$$[QT_INSTALL_PLUGINS]/
}
macx: {
TAS_TARGET_BIN=/usr/local/bin
TAS_TARGET_LIB=/usr/lib
TAS_TARGET_PLUGIN=$$[QT_INSTALL_PLUGINS]/
}

target.path = $$TAS_TARGET_PLUGIN/tasfixtures

#TAS_CORELIB=$$QTTAS_DIRECTORY/source/tascore/

DEPENDPATH += .
INCLUDEPATH += . #$$TAS_CORELIB
INCLUDEPATH += ../libtdrivereditor

# Input
HEADERS += visualizeraccessor.h
SOURCES += visualizeraccessor.cpp

DESTDIR = lib

INSTALLS += target

#LIBS += -L../TasDataModel/lib/ -ltasqtdatamodel
#LIBS += -L../../tascore/lib/ -lqttestability

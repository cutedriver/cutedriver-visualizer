#-------------------------------------------------
#
# Project created by QtCreator 2011-05-19T15:20:37
#
#-------------------------------------------------

include (../visualizer.pri)

TARGET = libtdriverfeatureditor

TEMPLATE = lib
CONFIG += shared
CONFIG += create_prl

DEFINES += LIBTDRIVERFEATUREDITOR_LIBRARY

# For libutil
CONFIG += link_prl
INCLUDEPATH += $$UTILLIBDIR
#LIBS += -L$$UTILLIBDIR -l$$UTIL_LIB
LIBS += -l$$UTIL_LIB

SOURCES += tdriver_featureditor.cpp \
    tdriver_featurabstractview.cpp \
    tdriver_featurfeatureview.cpp

HEADERS += tdriver_featureditor.h\
        libtdriverfeatureditor_global.h \
    tdriver_featurfeatureview.h \
    tdriver_featurabstractview.h

symbian {
    #Symbian specific definitions
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE9213BD3
    TARGET.CAPABILITY =
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = libtdriverfeatureditor.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/local/lib
    }
    INSTALLS += target
}

FORMS +=

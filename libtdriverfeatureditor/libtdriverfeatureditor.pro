#-------------------------------------------------
#
# Project created by QtCreator 2011-05-19T15:20:37
#
#-------------------------------------------------

include (../visualizer.pri)

TARGET = tdriverfeatureditor

TEMPLATE = lib
CONFIG += shared
CONFIG += create_prl

DEFINES += LIBTDRIVERFEATUREDITOR_LIBRARY

# For libutil
CONFIG += link_prl
INCLUDEPATH += $$UTILLIBDIR
#LIBS += -L$$UTILLIBDIR -l$$UTIL_LIB
LIBS += -l$$UTIL_LIB

QT += widgets gui xml

SOURCES += tdriver_featureditor.cpp \
    tdriver_featurabstractview.cpp \
    tdriver_featurfeatureview.cpp \
    tdriver_featurscenarioview.cpp \
    tdriver_standardfeaturmodel.cpp \
    tdriver_featurscenariostepview.cpp \
    tdriver_featurstepfileview.cpp \
    tdriver_featurstepdefview.cpp

HEADERS += tdriver_featureditor.h\
        libtdriverfeatureditor_global.h \
    tdriver_featurfeatureview.h \
    tdriver_featurabstractview.h \
    tdriver_featurscenarioview.h \
    tdriver_standardfeaturmodel.h \
    tdriver_featurscenariostepview.h \
    tdriver_featurstepfileview.h \
    tdriver_featurstepdefview.h



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


# install
unix:!symbian {
    target.path = /usr/lib
}
win32 {
    target.path = C:/tdriver/visualizer
}
INSTALLS += target

#-------------------------------------------------
#
# Project created by QtCreator 2018-12-03T13:46:36
#
#-------------------------------------------------
include(../../library.pri)

CONFIG(release, debug|release){
    OutDir=Release
    DEFINES += FILE_LOG
#    IDE_APP_PATH = $$dirname(PWD)/bin/
}
CONFIG(debug, debug|release){
    OutDir=Debug
    DEFINES += STD_LOG
#    IDE_APP_PATH = $$dirname(PWD)/Debug/
}

include($$ZCHX_3RD_PATH/zchx_ecdis/zchx_ecdis.pri)
include($$ZCHX_3RD_PATH/protobuf/protobuf.pri)
include($$ZCHX_3RD_PATH/ZeroMQ/zmq.pri)

QT       += core gui
CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += $${PROTOBUF_FILE_DIR}

TARGET = zchxMapTest
TEMPLATE = app
DESTDIR = $$IDE_APP_PATH

SOURCES += main.cpp \
    testmainwindow.cpp \
    testrotatewidget.cpp \
    data/zchxradardatachange.cpp \
    data/zchxradarechothread.cpp \
    data/zchxradarpointthread.cpp \
#    data/zchxradarrectthread.cpp \
    data/zchxaisthread.cpp \
    data/zchxradarutils.cpp \
    data/zchxfuntiontimer.cpp \
    testmapwatchdogthread.cpp \
    data/zchxradarlimitareathread.cpp \
    zchxfunction.cpp \
    ../../protobuf/2.6.1/ZCHXAISVessel.pb.cc \
    ../../protobuf/2.6.1/ZCHXRadarDataDef.pb.cc

FORMS += \
    testmainwindow.ui \
    testrotatewidget.ui

HEADERS += \
    testmainwindow.h \
    testrotatewidget.h \
    data/zchxradardatachange.h \
    data/zchxradarechothread.h \
    data/zchxradarpointthread.h \
#    data/zchxradarrectthread.h \
    data/zchxradarutils.h \
    data/zchxaisthread.h \
    data/zchxfuntiontimer.h \
    testmapwatchdogthread.h \
    data/zchxradarlimitareathread.h \
    zchxfunction.h \
    ../../protobuf/2.6.1/ZCHXAISVessel.pb.h \
    ../../protobuf/2.6.1/ZCHXRadarDataDef.pb.h


DEFINES += QT_MESSAGELOGCONTEXT


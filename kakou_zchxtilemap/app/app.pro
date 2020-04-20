#-------------------------------------------------
#
# Project created by QtCreator 2018-12-03T13:46:36
#
#-------------------------------------------------

CONFIG(release, debug|release):OutDir=Release
CONFIG(debug, debug|release):OutDir=Debug

PSFW_3RDPARTYPATH = $${PWD}/3rdParty
IDE_APP_PATH = $$dirname(PWD)/bin/

include($$PSFW_3RDPARTYPATH/zchx_ecdis/zchx_ecdis.pri)
include($$PSFW_3RDPARTYPATH/protobuf/protobuf.pri)
include($$PSFW_3RDPARTYPATH/ZeroMQ/zmq.pri)

QT       += core gui
CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = zchxMapTest
TEMPLATE = app
DESTDIR = $$IDE_APP_PATH

SOURCES += main.cpp \
    testmainwindow.cpp \
    testrotatewidget.cpp \
    radar/ZCHXRadar.pb.cc \
    radar/zchxradardatachange.cpp \
    radar/zchxradarechothread.cpp \
    radar/zchxradarpointthread.cpp \
    radar/zchxradarrectthread.cpp \
    radar/ZCHXRadarVideo.pb.cc \
    radar/zchxaisthread.cpp \
    radar/ZCHXAISVessel.pb.cc \
    radar/zchxradarutils.cpp \
    radar/zchxfuntiontimer.cpp \
    testmapwatchdogthread.cpp \
    radar/zchxradarlimitareathread.cpp

FORMS += \
    testmainwindow.ui \
    testrotatewidget.ui

HEADERS += \
    testmainwindow.h \
    testrotatewidget.h \
    radar/ZCHXRadar.pb.h \
    radar/zchxradardatachange.h \
    radar/zchxradarechothread.h \
    radar/zchxradarpointthread.h \
    radar/zchxradarrectthread.h \
    radar/zchxradarutils.h \
    radar/ZCHXRadarVideo.pb.h \
    radar/zchxaisthread.h \
    radar/ZCHXAISVessel.pb.h \
    radar/zchxfuntiontimer.h \
    testmapwatchdogthread.h \
    radar/zchxradarlimitareathread.h

DISTFILES += \
    radar/ZCHXRadar.proto \
    radar/ZCHXRadarVideo.proto

DEFINES += QT_MESSAGELOGCONTEXT


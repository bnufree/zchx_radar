#-------------------------------------------------
#
# Project created by QtCreator 2018-11-23T09:08:48
#
#-------------------------------------------------

ZCHX_3RD_PATH = ../3rdparty
if(!exists($$ZCHX_3RD_PATH)){
    message($$ZCHX_3RD_PATH does not exist)
}
DESTDIR = ../test/bin
IDE_APP_PATH = $${DESTDIR}


include($$ZCHX_3RD_PATH/ZeroMQ/zmq.pri)
include($$ZCHX_3RD_PATH/ProtoBuf/protobuf.pri)
include($$ZCHX_3RD_PATH/zchx_ecdis/zchx_ecdis.pri)



QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = zchx_radar_test
TEMPLATE = app

message($${IDE_APP_PATH})
SOURCES += main.cpp \
    radartestwindow.cpp \
    protobuf/ZCHXRadar.pb.cc \
    radarrecvthread.cpp

HEADERS  += \
    radartestwindow.h \
    protobuf/ZCHXRadar.pb.h \
    radarrecvthread.h

FORMS    += \
    radartestwindow.ui

DISTFILES += \
    protobuf/ZCHXRadar.proto

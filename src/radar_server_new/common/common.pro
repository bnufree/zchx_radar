#-------------------------------------------------
#
# Project created by QtCreator 2018-07-13T13:19:02
#
#-------------------------------------------------

QT       -= gui

TARGET = common
TEMPLATE = lib

DEFINES += COMMON_LIBRARY

SOURCES += common.cpp

HEADERS += common.h\
        common_global.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

#-------------------------------------------------
#
# ZCHX Radar Utils
# 雷达工具�?
#-------------------------------------------------
include(../library.pri)
TARGET_PATH = $${ZCHX_3RD_PATH}/RadarUtils

TARGET = zchx_radar_utils
TEMPLATE = lib
DESTDIR = $${TARGET_PATH}/bin

#Version
VERSION = 1.0.0

UTILSLIB_include_install.files += $$PWD/*.h
UTILSLIB_include_install.path = $${TARGET_PATH}/include
INSTALLS += UTILSLIB_include_install

HEADERS += \
    SineCosineLUT.h \
    DataQueue.h \
    Exception.h \
    FilePath.h \
    FileWatcher.h \
    Format.h \
    Threading.h \
    os-linux.h \
    osmacros.h \
    os-win32.h \
    UtilsGlobal.h \
    zchxRadarUtils.h

SOURCES += \
    SineCosineLUT.cpp \
    DataQueue.cpp \
    Exception.cpp \
    FilePath.cpp \
    FileWatcher.cpp \
    Format.cpp \
    Threading.cpp \
    zchxRadarUtils.cpp

HEADERS +=

SOURCES +=

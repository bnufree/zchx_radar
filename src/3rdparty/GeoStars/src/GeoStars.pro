#-------------------------------------------------
#
# ZCHX Radar GeoStars
#
#-------------------------------------------------

message($${PWD})
include(../library.pri)
TARGET_PATH = $${ZCHX_3RD_PATH}/GeoStars

TARGET = zchx_radar_geostars
TEMPLATE = lib
DESTDIR = $${TARGET_PATH}/bin

#Version
VERSION = 1.0.0

HEADERS += \
    geoStars.h \
    os-linux.h \
    osmacros.h \
    os-win32.h

SOURCES += \
    geoEllips.cpp \
    geoMag.cpp \
    geoPoint.cpp \
    geomainobj.cpp


GEOSTARSLIB_include_install.files += \
    geoStars.h \
    os-linux.h \
    osmacros.h \
    os-win32.h

GEOSTARSLIB_include_install.path = $${TARGET_PATH}/include
INSTALLS += GEOSTARSLIB_include_install

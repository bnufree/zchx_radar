#-------------------------------------------------
#
# ZCHX Radar Messages
# 雷达消息结构
#-------------------------------------------------

TARGET = zchx_radar_messages
TEMPLATE = lib

#Version
VERSION = 1.0.0


include(../../library.pri)
QT *= xml

include($$3RDPARTYPATH/boost/boost.pri)
include($$3RDPARTYPATH/protobuf/protobuf.pri)
include($$ZCHXRADARPATH/GeoStars/GeoStars.pri)
include($$ZCHXRADARPATH/Utils/Utils.pri)

HEADERS += \
    BinaryVideo.h \
    Extraction.h \
    GUID.h \
    Header.h \
    MetaTypeInfo.h \
    PRIMessage.h \
    RadarConfig.h \
    VideoConfig.h \
    TSPIConfig.h \
    RadarConfigFileWatcher.h \
    TSPI.h \
    Video.h \
    ZCHXRadar.pb.h

SOURCES += \
    BinaryVideo.cpp \
    Extraction.cpp \
    GUID.cpp \
    Header.cpp \
    MetaTypeInfo.cpp \
    PRIMessage.cpp \
    RadarConfig.cpp \
    VideoConfig.cpp \
    TSPIConfig.cpp \
    RadarConfigFileWatcher.cpp \
    TSPI.cpp \
    Video.cpp \
    ZCHXRadar.pb.cc


MESSAGESLIB_include_install.files += \
    BinaryVideo.h \
    Extraction.h \
    GUID.h \
    Header.h \
    MetaTypeInfo.h \
    PRIMessage.h \
    RadarConfig.h \
    VideoConfig.h \
    TSPIConfig.h \
    RadarConfigFileWatcher.h \
    TSPI.h \
    Video.h \
    ZCHXRadar.pb.h

MESSAGESLIB_include_install.path = $${ZCHXINCLUDEPATH}/Messages/
INSTALLS += MESSAGESLIB_include_install

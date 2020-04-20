


QT *= xml
INCLUDEPATH *= ../

message ($$INCLUDEPATH)

HEADERS += \
    $$PWD/BinaryVideo.h \
    $$PWD/Extraction.h \
    $$PWD/GUID.h \
    $$PWD/Header.h \
    $$PWD/MetaTypeInfo.h \
    $$PWD/PRIMessage.h \
    $$PWD/RadarConfig.h \
    $$PWD/VideoConfig.h \
    $$PWD/TSPIConfig.h \
    $$PWD/TSPI.h \
    $$PWD/Video.h \
    $$PWD/ZCHXRadar.pb.h

SOURCES += \
    $$PWD/BinaryVideo.cpp \
    $$PWD/Extraction.cpp \
    $$PWD/GUID.cpp \
    $$PWD/Header.cpp \
    $$PWD/MetaTypeInfo.cpp \
    $$PWD/PRIMessage.cpp \
    $$PWD/RadarConfig.cpp \
    $$PWD/VideoConfig.cpp \
    $$PWD/TSPIConfig.cpp \
    $$PWD/TSPI.cpp \
    $$PWD/Video.cpp \
    $$PWD/ZCHXRadar.pb.cc

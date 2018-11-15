#include($$3RDPARTYPATH/boost/boost.pri)
HEADERS += \
    $$PWD/Exception.h \
    $$PWD/FilePath.h \
    $$PWD/FileWatcher.h \
    $$PWD/Format.h \
    $$PWD/SineCosineLUT.h \
    $$PWD/DataQueue.h \
#    $$PWD/Threading.h \
    $$PWD/Utils.h

SOURCES += \
    $$PWD/Exception.cpp \
    $$PWD/FilePath.cpp \
    $$PWD/FileWatcher.cpp \
    $$PWD/Format.cpp \
    $$PWD/SineCosineLUT.cpp \
    $$PWD/DataQueue.cpp \
#    $$PWD/Threading.cpp \
    $$PWD/Utils.cpp

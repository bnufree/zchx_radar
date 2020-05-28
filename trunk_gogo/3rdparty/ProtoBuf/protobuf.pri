mingw{
    #PB_Version = 2.6.1-msys2
#INCLUDEPATH += $$PWD/$$PB_Version/include
#LIBS += -L$$PWD/$$PB_Version/Release -llibprotobuf

PB_Version = 3.12.2
INCLUDEPATH += $$PWD/$$PB_Version/include
LIBS += -L$$PWD/$$PB_Version/lib -llibprotobuf

ProtoBuf_install.files += $$PWD/$$PB_Version/bin/libprotobuf.dll
ProtoBuf_install.path = $$IDE_APP_PATH/
INSTALLS += ProtoBuf_install
}

unix{
    INCLUDEPATH += /usr/lcoal/include
    LIBS += -L/usr/lib/ -lprotobuf
}

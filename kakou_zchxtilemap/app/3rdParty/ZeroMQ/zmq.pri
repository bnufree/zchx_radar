CONFIG(release, debug|release):BuildType=Release
CONFIG(debug, debug|release):BuildType=Debug

linux*{
    ZMQ_Version = 4.1.4-linux
    INCLUDEPATH += $$PWD/$$ZMQ_Version/include
    LIBS += -L$$PWD/$$ZMQ_Version/lib -lzmq

    zmqlibrary.target = $$IDE_APP_PATH/libzmq.so.5
    zmqlibrary.commands = $$PATH_PYTHON $$IDE_ROOT/CompileHelper.py COPYDIR $$PWD/$$ZMQ_Version/lib $$IDE_APP_PATH
    zmqlibrary.depends = $$IDE_ROOT/CompileHelper.py
    QMAKE_EXTRA_TARGETS += zmqlibrary
    PRE_TARGETDEPS += $${zmqlibrary.target}
}
else:win32{
    ZMQ_Version = 4.2.3_x86
#    contains(QMAKE_HOST.arch, x86_64):ZMQ_Version = 4.2.3_x64
    contains(QMAKE_HOST.arch, x86_64):ZMQ_Version = mingw
    INCLUDEPATH += $$PWD/$$ZMQ_Version/include
    LIBS += -L$$PWD/$$ZMQ_Version/Release -llibzmq
    #zmqlibrary.target = $$IDE_APP_PATH/libzmq.dll
    #zmqlibrary.depends = $$PWD/$$ZMQ_Version/bin/libzmq.dll
    #zmqlibrary.commands = $(COPY_FILE) \"$$replace(zmqlibrary.depends, /, $$QMAKE_DIR_SEP)\" \"$$replace(zmqlibrary.target, /, $$QMAKE_DIR_SEP)\"
    #QMAKE_EXTRA_TARGETS += zmqlibrary
    #PRE_TARGETDEPS += $${zmqlibrary.target}

    zmqinstall.files += $$PWD/$$ZMQ_Version/$$BuildType/*.dll
#    zmqinstall.files += $$PWD/$$ZMQ_Version/bin/*.dll
    zmqinstall.path = $$IDE_APP_PATH
    INSTALLS *= zmqinstall
}

INCLUDEPATH += $$PWD/include

LIBS += $$PWD/lib/Record.lib
Record_install.files += $$PWD/bin/Record.dll
Record_install.path = $$IDE_APP_PATH/
INSTALLS += Record_install

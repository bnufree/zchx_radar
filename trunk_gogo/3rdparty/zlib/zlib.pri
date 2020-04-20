INCLUDEPATH += $$PWD/include
LIBS += -L$$PWD/lib/ -llibz
Zlib_install.files += $$PWD/bin/zlib1.dll
Zlib_install.path = $$IDE_APP_PATH
INSTALLS +=Zlib_install

INCLUDEPATH += $$PWD/include
LIBS += -L$$PWD/lib/Release -llibais_analysis
Ais_install.files += $$PWD/bin/Release/ais_analysis.dll
Ais_install.path = $$IDE_APP_PATH/
INSTALLS += Ais_install

INCLUDEPATH += $$PWD/include
LIBS += -L$$PWD/bin -llibzchx_radar_geostars
Geo_install.files += $$PWD/bin/zchx_radar_geostars.dll
Geo_install.path = $$IDE_APP_PATH/
INSTALLS += Geo_install

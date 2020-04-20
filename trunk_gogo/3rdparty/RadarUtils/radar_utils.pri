INCLUDEPATH += $$PWD/include
LIBS += -L$$PWD/bin -llibzchx_radar_utils
RadarUtils_install.files += $$PWD/bin/zchx_radar_utils.dll
RadarUtils_install.path = $$IDE_APP_PATH/
INSTALLS += RadarUtils_install

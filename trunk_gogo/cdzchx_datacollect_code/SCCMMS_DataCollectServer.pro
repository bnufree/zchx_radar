#-------------------------------------------------
#
# Project created by QtCreator 2017-02-10T16:19:16
#
#-------------------------------------------------

ZCHX_3RD_PATH = ../3rdparty
SIDE_CAR_PARSE_DIR = $$PWD/side_car_parse/
if(!exists($$ZCHX_3RD_PATH)){
    message($$ZCHX_3RD_PATH does not exist)
}

CONFIG += c++11 c++14
TARGET = radar_data_collect_server
TEMPLATE = app
DESTDIR = ../bin
IDE_APP_PATH= $$DESTDIR

include($$ZCHX_3RD_PATH/ZeroMQ/zmq.pri)
include($$ZCHX_3RD_PATH/ProtoBuf/protobuf.pri)
#include($$ZCHX_3RD_PATH/zchx_ais/zchx_ais.pri)
#include($$ZCHX_3RD_PATH/zchx_radar/zchx_radar.pri)
#include($$ZCHX_3RD_PATH/zlib/zlib.pri)
#include($$ZCHX_3RD_PATH/videoToTrack/videoToTrack.pri)
#include($$ZCHX_3RD_PATH/GeoStars/geo_stars.pri)
#include($$ZCHX_3RD_PATH/RadarUtils/radar_utils.pri)
#include($$ZCHX_3RD_PATH/QtXlsx/QtXlsx.pri)
include($$ZCHX_3RD_PATH/opencv/opencv.pri)

DEFINES += WIN32_LEAN_AND_MEAN
QT += core gui network websockets positioning xml serialport svg
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH *= $$ZCHX_3RD_PATH/ $$SIDE_CAR_PARSE_DIR

mingw{
    LIBS += -lws2_32 -llibboost_system-mt -llibboost_regex-mt
    LIBS += -lpsapi
}




SOURCES += main.cpp\
    Log.cpp \
    aislabel.cpp \
    profiles.cpp \
    dataserverutils.cpp \
    ZmqMonitorThread.cpp \
    ais/zchxaisdataserver.cpp \
    ais/ZCHXAISVessel.pb.cc \
    ais/zchxaisdataprocessor.cpp \
    ais_radar/zchxradardataserver.cpp \
    ais_radar/zchxfunction.cpp \
    ais_radar/ZCHXRadar.pb.cc \
    ais_radar/ZCHXRadarVideo.pb.cc \
    ais_radar/zchxradaraissetting.cpp \
    ais_radar/zchxanalysisandsendradar.cpp \
    ais_radar/qradarparamsetting.cpp \
    ais_radar/qradarstatussettingwidget.cpp \    
    ais_radar/updatevideoudpthread.cpp \
    dialog_log.cpp \
    dialog_set.cpp \
    dialog_cli.cpp \
    up_video_pthread.cpp \
    up_ais_pthread.cpp \
    scrollarea.cpp \
    myLabel.cpp \
    globlefun/glogfunction.cpp \
    protobuf/protobufdataprocessor.cpp \
    comdata/comdataworker.cpp \
    protobuf/TWQMSComData.pb.cc \
    comdata/comdatamgr.cpp \
    comdata/comconfigwidget.cpp \
    comdata/comselectedcombobox.cpp \
    dataout/comdatapubworker.cpp \
    protobuf/protobufdatadisplaywidget.cpp \
    comdata/comparser.cpp \
    ais_setting.cpp \
    drawaistrack.cpp \
    radar_control.cpp \
    dialog_help.cpp \
    ais_radar/zchxMulticastDataSocket.cpp \
    ais_radar/zchxRadarVideoRecvThread.cpp \
    float_setting.cpp \
    zchxmainwindow.cpp \
    aisbaseinfosetting.cpp \
    zchxradarinteface.cpp \
    ais_radar/ZCHXBd.pb.cc \
    beidoudata.cpp \
    fusedatautil.cpp \
    util.cpp \
    ais_radar/zchxRadarRectExtraction.cpp \
    zchxradaroptwidget.cpp \
    zchxradarctrlbtn.cpp \
    ais_radar/zchxradarvideoprocessor.cpp \
    ais_radar/zchxradartargettrack.cpp \
    ais_radar/zchxradarcommon.cpp \
    dataout/zchxdataoutputservermgr.cpp \
    dataout/zchxdataoutputserverthread.cpp \
    ais/zchxaischartworker.cpp \
    ais/zchxaisdatacollector.cpp \
    ais/zchxaisdataclient.cpp \
    zchxsimulatethread.cpp \
    ais_radar/zchxlowranceradardataserver.cpp \
    zchxhostsetting.cpp \
    ais/ais.cpp \
    ais/ais1_2_3.cpp \
    ais/ais4_11.cpp \
    ais/ais5.cpp \
    ais/ais6.cpp \
    ais/ais7_13.cpp \
    ais/ais8.cpp \
    ais/ais8_001_22.cpp \
    ais/ais8_001_26.cpp \
    ais/ais8_366_22.cpp \
    ais/ais9.cpp \
    ais/ais10.cpp \
    ais/ais12.cpp \
    ais/ais14.cpp \
    ais/ais15.cpp \
    ais/ais16.cpp \
    ais/ais17.cpp \
    ais/ais18.cpp \
    ais/ais19.cpp \
    ais/ais20.cpp \
    ais/ais21.cpp \
    ais/ais22.cpp \
    ais/ais23.cpp \
    ais/ais24.cpp \
    ais/ais25.cpp \
    ais/ais26.cpp \
    ais/ais27.cpp

HEADERS  += \
    Log.h \
    aislabel.h \
    profiles.h \
    dataserverutils.h \
    ZmqMonitorThread.h \
    ais/zchxaisdataserver.h \
    ais/zchxaisdataprocessor.h \
    ais/ZCHXAISVessel.pb.h \
    ais_radar/zchxradardataserver.h \
    ais_radar/zchxfunction.h \
    ais_radar/ZCHXRadar.pb.h \
    ais_radar/ZCHXRadarVideo.pb.h \
    ais_radar/BR24.hpp \    
    ais_radar/zchxradaraissetting.h \
    ais_radar/zchxanalysisandsendradar.h \
    ais_radar/qradarparamsetting.h \
    ais_radar/radarccontroldefines.h \
    ais_radar/qradarstatussettingwidget.h \   
    ais_radar/updatevideoudpthread.h \
    dialog_log.h \
    dialog_set.h \
    dialog_cli.h \
    up_video_pthread.h \
    up_ais_pthread.h \
    scrollarea.h \
    myLabel.h \
    globlefun/glogfunction.h \
    protobuf/protobufdataprocessor.h \
    common.h \
    comdata/comdataworker.h \
    protobuf/TWQMSComData.pb.h \
    comdata/comdatamgr.h \
    comdata/comconfigwidget.h \
    comdata/comselectedcombobox.h \
    dataout/comdatapubworker.h \
    protobuf/protobufdatadisplaywidget.h \
    comdata/comdefines.h \
    comdata/comparser.h \
    ais_setting.h \
    drawaistrack.h \
    radar_control.h \
    dialog_help.h \
    ais_radar/zchxMulticastDataSocket.h \
    ais_radar/zchxRadarVideoRecvThread.h \
    float_setting.h \
    zchxmainwindow.h \
    aisbaseinfosetting.h \
    zchxradarinteface.h \
    ais_radar/ZCHXBd.pb.h \
    beidoudata.h \
    fusedatautil.h \
    util.h \
    ais_radar/zchxradarcommon.h \
    ais_radar/zchxRadarRectExtraction.h \
    zchxradaroptwidget.h \
    zchxradarctrlbtn.h \
    ais_radar/zchxradarvideoprocessor.h \
    ais_radar/zchxradartargettrack.h \
    dataout/zchxdataoutputservermgr.h \
    dataout/zchxdataoutputserverthread.h \
    ais/zchxaischartworker.h \
    ais/zchxaisdatacollector.h \
    ais/zchxaisdataclient.h \
    zchxsimulatethread.h \
    ais_radar/zchxlowranceradardataserver.h \
    zchxhostsetting.h \
    ais/ais.h \
    ais/ais8_001_22.h



mingw{
    SOURCES += zchxmapmonitorthread.cpp
    HEADERS += zchxmapmonitorthread.h
}

FORMS    += \
    ais_radar/zchxradaraissetting.ui \
    ais_radar/qradarparamsetting.ui \
    ais_radar/qradarstatussettingwidget.ui \
    dialog_log.ui \
    dialog_set.ui \
    dialog_cli.ui \
    comdata/comconfigwidget.ui \
    protobuf/protobufdatadisplaywidget.ui \
    ais_setting.ui \
    radar_control.ui \
    dialog_help.ui \
    float_setting.ui \
    zchxmainwindow.ui \
    aisbaseinfosetting.ui \
    zchxradarinteface.ui \
    beidoudata.ui \
    zchxradaroptwidget.ui \
    zchxhostsetting.ui


DISTFILES += \
    SCCMMSComData.proto \
    ais_radar/ZCHXRadar.proto \
    ais_radar/ZCHXRadarVideo.proto \
    ais/ZCHXAISVessel.proto \
    zmq/SCCMMSComData.proto \
    increase.png \
    ais_radar/ZCHXBd.proto
RC_FILE  =  app.rc

RESOURCES += \
    res.qrc

SUBDIRS += \
    side_car_parse/Messages/Messages.pro

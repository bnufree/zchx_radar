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
include($$ZCHX_3RD_PATH/zchx_ais/zchx_ais.pri)
include($$ZCHX_3RD_PATH/zchx_radar/zchx_radar.pri)
include($$ZCHX_3RD_PATH/zlib/zlib.pri)
#include($$ZCHX_3RD_PATH/videoToTrack/videoToTrack.pri)
#include($$ZCHX_3RD_PATH/GeoStars/geo_stars.pri)
#include($$ZCHX_3RD_PATH/RadarUtils/radar_utils.pri)
#include($$ZCHX_3RD_PATH/QtXlsx/QtXlsx.pri)
include($$ZCHX_3RD_PATH/opencv/opencv.pri)

DEFINES += WIN32_LEAN_AND_MEAN
QT += core gui network websockets positioning xml serialport axcontainer svg
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH *= $$ZCHX_3RD_PATH/ $$SIDE_CAR_PARSE_DIR

LIBS += -lws2_32 -llibboost_system-mt
LIBS += -lpsapi




SOURCES += main.cpp\
    Log.cpp \
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
    ais_radar/zchxradarechodatachange.cpp \
    ais_radar/zmqradarechothread.cpp \
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
    aisLabel.cpp \
    radar_control.cpp \
    dialog_help.cpp \
#    side_car_parse/Algorithms/ABTrack.cc \
#    side_car_parse/Algorithms/ABTracker.cc \
#    side_car_parse/Algorithms/Centroid.cc \
#    side_car_parse/Algorithms/CFAR.cc \
#    side_car_parse/Algorithms/ExtractWithCentroiding.cc \
#    side_car_parse/Algorithms/ImageSegmentation.cc \
#    side_car_parse/Algorithms/OSCFAR.cc \
#    side_car_parse/Algorithms/PastBuffer.cc \
#    side_car_parse/Algorithms/Row.cc \
#    side_car_parse/Algorithms/ScanCorrelator.cc \
#    side_car_parse/Algorithms/SegmentedTargetImage.cc \
#    side_car_parse/Algorithms/TargetSize.cc \
#    side_car_parse/Algorithms/Threshold.cc \
#    side_car_parse/Algorithms/TrackInitiator.cc \
#    side_car_parse/Algorithms/TrackMaintainer.cc \
#    side_car_parse/Algorithms/UnitVector.cc \
#    side_car_parse/Algorithms/Vector.cc \
#    side_car_parse/Messages/BinaryVideo.cpp \
#    side_car_parse/Messages/Extraction.cpp \
#    side_car_parse/Messages/Header.cpp \
#    side_car_parse/Messages/MetaTypeInfo.cpp \
#    side_car_parse/Messages/PRIMessage.cpp \
#    side_car_parse/Messages/RadarConfig.cpp \
#    side_car_parse/Messages/Track.cc \
#    side_car_parse/Messages/TSPI.cpp \
#    side_car_parse/Messages/TSPIConfig.cpp \
#    side_car_parse/Messages/Video.cpp \
#    side_car_parse/Messages/VideoConfig.cpp \
#    side_car_parse/Messages/GGUID.cpp \
    ais_radar/zchxMulticastDataSocket.cpp \
    ais_radar/zchxRadarVideoRecvThread.cpp \
#    ais_radar/zchxradarextractthread.cpp \
    float_setting.cpp \
    zchxmainwindow.cpp \
    aisbaseinfosetting.cpp \
#    ais_radar/zchxvideorects.cpp \
    zchxradarinteface.cpp \
    ais_radar/ZCHXBd.pb.cc \
    beidoudata.cpp \
    fusedatautil.cpp \
    util.cpp \
    ais_radar/zchxRadarRectExtraction.cpp \
    zchxradaroptwidget.cpp \
    zchxradarctrlbtn.cpp \
    zchxmapmonitorthread.cpp \
    ais_radar/zchxradarvideoprocessor.cpp \
    ais_radar/zchxradartargettrack.cpp \
    ais_radar/zchxradarcommon.cpp \
    dataout/zchxdataoutputservermgr.cpp \
    dataout/zchxdataoutputserverthread.cpp \
    ais/zchxaischartworker.cpp \
    ais/zchxaisdatacollector.cpp \
    ais/zchxaisdataclient.cpp \
    zchxsimulatethread.cpp

HEADERS  += \
    Log.h \
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
    ais_radar/zchxradarechodatachange.h \
    ais_radar/zmqradarechothread.h \
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
    aisLabel.h \
    radar_control.h \
    dialog_help.h \
#    side_car_parse/Algorithms/ABTrack.h \
#    side_car_parse/Algorithms/ABTracker.h \
#    side_car_parse/Algorithms/ABTracker_defaults.h \
#    side_car_parse/Algorithms/Centroid.h \
#    side_car_parse/Algorithms/CFAR.h \
#    side_car_parse/Algorithms/CFAR_defaults.h \
#    side_car_parse/Algorithms/ExtractWithCentroiding.h \
#    side_car_parse/Algorithms/ExtractWithCentroiding_defaults.h \
#    side_car_parse/Algorithms/ImageDataTypes.h \
#    side_car_parse/Algorithms/ImageSegmentation.h \
#    side_car_parse/Algorithms/OSCFAR.h \
#    side_car_parse/Algorithms/OSCFAR_defaults.h \
#    side_car_parse/Algorithms/PastBuffer.h \
#    side_car_parse/Algorithms/Row.h \
#    side_car_parse/Algorithms/ScanCorrelator.h \
#    side_car_parse/Algorithms/ScanCorrelator_defaults.h \
#    side_car_parse/Algorithms/ScanLine.h \
#    side_car_parse/Algorithms/SegmentedTargetImage.h \
#    side_car_parse/Algorithms/TargetImage.h \
#    side_car_parse/Algorithms/TargetSize.h \
#    side_car_parse/Algorithms/Threshold.h \
#    side_car_parse/Algorithms/Threshold_defaults.h \
#    side_car_parse/Algorithms/TrackInitiator.h \
#    side_car_parse/Algorithms/TrackInitiator_defaults.h \
#    side_car_parse/Algorithms/TrackMaintainer.h \
#    side_car_parse/Algorithms/UnitVector.h \
#    side_car_parse/Algorithms/Vector.h \
#    side_car_parse/Algorithms/VideoStorage.h \
#    side_car_parse/Messages/BinaryVideo.h \
#    side_car_parse/Messages/Extraction.h \
#    side_car_parse/Messages/Header.h \
#    side_car_parse/Messages/MessagesGlobal.h \
#    side_car_parse/Messages/MetaTypeInfo.h \
#    side_car_parse/Messages/PRIMessage.h \
#    side_car_parse/Messages/RadarConfig.h \
#    side_car_parse/Messages/Track.h \
#    side_car_parse/Messages/TSPI.h \
#    side_car_parse/Messages/TSPIConfig.h \
#    side_car_parse/Messages/Video.h \
#    side_car_parse/Messages/VideoConfig.h \
#    side_car_parse/Messages/GGUID.h \
    ais_radar/zchxMulticastDataSocket.h \
    ais_radar/zchxRadarVideoRecvThread.h \
#    ais_radar/zchxradarextractthread.h \
    float_setting.h \
    zchxmainwindow.h \
    aisbaseinfosetting.h \
#    ais_radar/zchxvideorects.h \
    zchxradarinteface.h \
    ais_radar/ZCHXBd.pb.h \
    beidoudata.h \
    fusedatautil.h \
    util.h \
    ais_radar/zchxradarcommon.h \
    ais_radar/zchxRadarRectExtraction.h \
    zchxradaroptwidget.h \
    zchxradarctrlbtn.h \
    zchxmapmonitorthread.h \
    ais_radar/zchxradarvideoprocessor.h \
    ais_radar/zchxradartargettrack.h \
    dataout/zchxdataoutputservermgr.h \
    dataout/zchxdataoutputserverthread.h \
    ais/zchxaischartworker.h \
    ais/zchxaisdatacollector.h \
    ais/zchxaisdataclient.h \
    zchxsimulatethread.h





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
    zchxradaroptwidget.ui


DISTFILES += \
    SCCMMSComData.proto \
    ais_radar/ZCHXRadar.proto \
    ais_radar/ZCHXRadarVideo.proto \
    ais/ZCHXAISVessel.proto \
    zmq/SCCMMSComData.proto \
    increase.png \
#    side_car_parse/Algorithms/ABTracker.axml \
#    side_car_parse/Algorithms/CFAR.axml \
#    side_car_parse/Algorithms/ExtractWithCentroiding.axml \
#    side_car_parse/Algorithms/OSCFAR.axml \
#    side_car_parse/Algorithms/ScanCorrelator.axml \
#    side_car_parse/Algorithms/Threshold.axml \
#    side_car_parse/Algorithms/TrackInitiator.axml \
#    side_car_parse/Algorithms/TrackMaintainer.axml \
#    side_car_parse/Messages/Messages.pri \
#    side_car_parse/Messages/MessagesSrc.pri \
#    side_car_parse/Messages/ZCHXRadar2.proto \
    ais_radar/ZCHXBd.proto
RC_FILE  =  app.rc

RESOURCES += \
    res.qrc

SUBDIRS += \
    side_car_parse/Messages/Messages.pro

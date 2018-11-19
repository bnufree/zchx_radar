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

include($$ZCHX_3RD_PATH/ZeroMQ/zmq.pri)
include($$ZCHX_3RD_PATH/ProtoBuf/protobuf.pri)
include($$ZCHX_3RD_PATH/zchx_ais/zchx_ais.pri)
include($$ZCHX_3RD_PATH/zchx_radar/zchx_radar.pri)
include($$ZCHX_3RD_PATH/zlib/zlib.pri)
include($$ZCHX_3RD_PATH/videoToTrack/videoToTrack.pri)
include($$ZCHX_3RD_PATH/GeoStars/geo_stars.pri)
include($$ZCHX_3RD_PATH/RadarUtils/radar_utils.pri)

DEFINES += WIN32_LEAN_AND_MEAN
QT += core gui network websockets positioning xml serialport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH *= $$ZCHX_3RD_PATH/ $$SIDE_CAR_PARSE_DIR
message($$INCLUDEPATH)

CONFIG += c++11 c++14
TARGET = Data_Collect_Server
TEMPLATE = app
DESTDIR = ../build2/bin
LIBS += -lws2_32 -llibboost_system-mt

SOURCES += main.cpp\
        mainwindow.cpp \
    Log.cpp \
    profiles.cpp \
    dataserverutils.cpp \
    ZmqMonitorThread.cpp \
    protobuf/ZCHXRadar.pb.cc \
    protobuf/ZCHXRadarVideo.pb.cc \
    protobuf/ZCHXAISVessel.pb.cc \
    ais_radar/zchxdrawvideorunnable.cpp \
    ais_radar/zchxradarprocessthread.cpp \
    collectserver.cpp \
    comsendtread.cpp \
    datacomconfigdlg.cpp \
    datacomconfiglistdlg.cpp \
    datauploadserver.cpp \
    haworker.cpp \
    heartthread.cpp \
    qcomportselectedcombobox.cpp \
    qdatatcpreceiver.cpp \
    qdeviceparamlabel.cpp \
    recivecomdata.cpp \
    SCCMMSComData.pb.cc \
    systemconfigwidget.cpp \
    webserver.cpp \
    side_car_parse/Algorithms/OSCFAR.cc \
    side_car_parse/Algorithms/CFAR.cc \
    side_car_parse/Algorithms/Threshold.cc \
    side_car_parse/Algorithms/PastBuffer.cc \
    side_car_parse/Algorithms/ScanCorrelator.cc \
    side_car_parse/Algorithms/TrackInitiator.cc \
    side_car_parse/Algorithms/TrackMaintainer.cc \
    side_car_parse/Algorithms/ABTracker.cc \
    side_car_parse/Algorithms/UnitVector.cc \
    side_car_parse/Algorithms/Vector.cc \
    side_car_parse/Algorithms/ABTrack.cc \
    side_car_parse/Algorithms/Centroid.cc \
    side_car_parse/Algorithms/ExtractWithCentroiding.cc \
    side_car_parse/Algorithms/ImageSegmentation.cc \
    side_car_parse/Algorithms/Row.cc \
    side_car_parse/Algorithms/SegmentedTargetImage.cc \
    side_car_parse/Algorithms/TargetSize.cc \
    ais_radar/zchxaisdataserver.cpp \
    ais_radar/zchxradardataserver.cpp \
    ais_radar/zchxdrawradarvideo.cpp \
    ais_radar/zchxfunction.cpp \
    ais_radar/zchxradaraissetting.cpp \
    ais_radar/zchxradarechodatachange.cpp \
    ais_radar/zmqradarechothread.cpp \
    ais_radar/zxhcprocessechodata.cpp \
    ais_radar/zchxanalysisandsendradar.cpp \
    ais_radar/zchxgettrackprocess.cpp \
    ais_radar/qradarparamsetting.cpp \
    ais_radar/qradarstatussettingwidget.cpp \
    ais_radar/zchxaisdataprocessor.cpp \
    side_car_parse/Messages/BinaryVideo.cpp \
    side_car_parse/Messages/Extraction.cpp \
    side_car_parse/Messages/GUID.cpp \
    side_car_parse/Messages/Header.cpp \
    side_car_parse/Messages/MetaTypeInfo.cpp \
    side_car_parse/Messages/PRIMessage.cpp \
    side_car_parse/Messages/RadarConfig.cpp \
    side_car_parse/Messages/TSPI.cpp \
    side_car_parse/Messages/TSPIConfig.cpp \
    side_car_parse/Messages/Video.cpp \
    side_car_parse/Messages/VideoConfig.cpp \
    side_car_parse/Messages/Track.cc \
    ais_radar/VideoDataProcessWorker.cpp \
    ais_radar/TargetExtractionWorker.cpp \
    ais_radar/zchxRadarHeartWorker.cpp \
    ais_radar/zchxRadarCtrlWorker.cpp \
    ais_radar/zchxRadarReportWorker.cpp \
    ais_radar/zchxMulticastDataSocket.cpp \
    ais_radar/VideoDataRecvThread.cpp

HEADERS  += mainwindow.h \
    Log.h \
    profiles.h \
    dataserverutils.h \
    ZmqMonitorThread.h \
    ais_radar/zchxaisdataserver.h \
    ais_radar/zchxradardataserver.h \
    ais_radar/zchxdrawradarvideo.h \
    ais_radar/zchxfunction.h \
    ais_radar/BR24.hpp \
    ais_radar/zchxradaraissetting.h \
    ais_radar/zchxradarechodatachange.h \
    ais_radar/zmqradarechothread.h \
    ais_radar/zxhcprocessechodata.h \
    ais_radar/zchxanalysisandsendradar.h \
    ais_radar/zchxgettrackprocess.h \
    ais_radar/qradarparamsetting.h \
#    radar_control_1/lowrance_control.hpp \
#    radar_control_1/radar_config.hpp \
#    radar_control_1/radar_control.hpp \
    ais_radar/radarccontroldefines.h \
    ais_radar/qradarstatussettingwidget.h \
    ais_radar/zchxaisdataprocessor.h \
    protobuf/ZCHXRadar.pb.h \
    protobuf/ZCHXRadarVideo.pb.h \
    protobuf/ZCHXAISVessel.pb.h \
    ais_radar/zchxdrawvideorunnable.h \
    ais_radar/zchxradarprocessthread.h \    
    collectserver.h \
    comsendtread.h \
    datacomconfigdlg.h \
    datacomconfiglistdlg.h \
    datauploadserver.h \
    haworker.h \
    heartthread.h \
    qcomportselectedcombobox.h \
    qdatatcpreceiver.h \
    qdeviceparamlabel.h \
    recivecomdata.h \
    SCCMMSComData.pb.h \
    systemconfigsettingdefines.h \
    systemconfigwidget.h \
    webserver.h \
    common.h \
    side_car_parse/Messages/BinaryVideo.h \
    side_car_parse/Messages/Extraction.h \
    side_car_parse/Messages/GUID.h \
    side_car_parse/Messages/Header.h \
    side_car_parse/Messages/MessagesGlobal.h \
    side_car_parse/Messages/MetaTypeInfo.h \
    side_car_parse/Messages/PRIMessage.h \
    side_car_parse/Messages/RadarConfig.h \
    side_car_parse/Messages/TSPI.h \
    side_car_parse/Messages/TSPIConfig.h \
    side_car_parse/Messages/Video.h \
    side_car_parse/Messages/VideoConfig.h \
    side_car_parse/Messages/Track.h \
    side_car_parse/Algorithms/OSCFAR.h \
    side_car_parse/Algorithms/OSCFAR_defaults.h \
    side_car_parse/Algorithms/CFAR.h \
    side_car_parse/Algorithms/CFAR_defaults.h \
    side_car_parse/Algorithms/Threshold.h \
    side_car_parse/Algorithms/Threshold_defaults.h \
    side_car_parse/Algorithms/PastBuffer.h \
    side_car_parse/Algorithms/ScanCorrelator.h \
    side_car_parse/Algorithms/ScanCorrelator_defaults.h \
    side_car_parse/Algorithms/TrackInitiator.h \
    side_car_parse/Algorithms/TrackMaintainer.h \
    side_car_parse/Algorithms/TrackInitiator_defaults.h \
    side_car_parse/Algorithms/ABTracker.h \
    side_car_parse/Algorithms/ABTracker_defaults.h \
    side_car_parse/Algorithms/UnitVector.h \
    side_car_parse/Algorithms/Vector.h \
    side_car_parse/Algorithms/ABTrack.h \
    side_car_parse/Algorithms/Centroid.h \
    side_car_parse/Algorithms/ExtractWithCentroiding.h \
    side_car_parse/Algorithms/ExtractWithCentroiding_defaults.h \
    side_car_parse/Algorithms/ImageDataTypes.h \
    side_car_parse/Algorithms/ImageSegmentation.h \
    side_car_parse/Algorithms/Row.h \
    side_car_parse/Algorithms/ScanLine.h \
    side_car_parse/Algorithms/SegmentedTargetImage.h \
    side_car_parse/Algorithms/TargetImage.h \
    side_car_parse/Algorithms/TargetSize.h \
    side_car_parse/Algorithms/VideoStorage.h \
    ais_radar/VideoDataProcessWorker.h \
    ais_radar/TargetExtractionWorker.h \
    ais_radar/zchxRadarHeartWorker.h \
    ais_radar/zchxRadarCtrlWorker.h \
    ais_radar/zchxRadarReportWorker.h \
    ais_radar/zchxMulticastDataSocket.h \
    ais_radar/VideoDataRecvThread.h

FORMS    += mainwindow.ui \
    ais_radar/zchxradaraissetting.ui \
    ais_radar/qradarparamsetting.ui \
    ais_radar/qradarstatussettingwidget.ui \
    datacomconfigdlg.ui \
    datacomconfiglistdlg.ui \
    systemconfigwidget.ui

DISTFILES += \
    SCCMMSComData.proto \
    protobuf/ZCHXRadar.proto \
    protobuf/ZCHXRadarVideo.proto \
    protobuf/ZCHXAISVessel.proto \
    side_car_parse/Algorithms/ExtractWithCentroiding.axml \
    side_car_parse/Algorithms/OSCFAR.axml \
    side_car_parse/Algorithms/CFAR.axml \
    side_car_parse/Algorithms/Threshold.axml \
    side_car_parse/Algorithms/ScanCorrelator.axml \
    side_car_parse/Algorithms/TrackInitiator.axml \
    side_car_parse/Algorithms/TrackMaintainer.axml \
    side_car_parse/Algorithms/ABTracker.axml
RC_FILE  =  app.rc

RESOURCES += \
    res.qrc

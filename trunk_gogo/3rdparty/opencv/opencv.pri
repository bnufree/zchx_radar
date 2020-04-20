
ZCHX_OPENCV_NAME        = opencv
ZCHX_OPENCV_VERSION     = 4.2.1
######################################################################
# 3rdparty path
######################################################################

#ZCHX_ECDIS_PATH = $${PSFW_3RDPARTYPATH}/$${ZCHX_ECDIS_NAME}/$${ZCHX_ECDIS_VERSION}
ZCHX_OPENCV_PATH = $${PWD}/$${ZCHX_OPENCV_VERSION}

exists( $${ZCHX_OPENCV_PATH} ) {
message("find ZCHX_OPENCV DIR   =======================" ++ $$ZCHX_OPENCV_PATH)
    ######################################################################
    # Include library
    ######################################################################
    ZCHX_OPENCV_HEADERS   = $${ZCHX_OPENCV_PATH}/include
    ZCHX_OPENCV_LIBS      = $${ZCHX_OPENCV_PATH}/x64/mingw/lib

    INCLUDEPATH += $${ZCHX_OPENCV_HEADERS}

    CONFIG(release, debug|release) {
        LIBS +=-L$${ZCHX_OPENCV_LIBS} \
                     -llibopencv_calib3d420 \
-llibopencv_core420 \
#-llibopencv_dnn420 \
#-llibopencv_features2d420 \
#-llibopencv_flann420 \
#-llibopencv_gapi420 \
#-llibopencv_highgui420 \
-llibopencv_imgcodecs420 \
-llibopencv_imgproc420 \
#-llibopencv_ml420 \
#-llibopencv_objdetect420 \
#-llibopencv_photo420 \
#-llibopencv_stitching420 \
#-llibopencv_video420 \
#-llibopencv_videoio420 \

        ZCHX_OPENCV_install.files += $${ZCHX_OPENCV_PATH}/x64/mingw/bin/libopencv_core420.dll
        ZCHX_OPENCV_install.files += $${ZCHX_OPENCV_PATH}/x64/mingw/bin/libopencv_imgcodecs420.dll
        ZCHX_OPENCV_install.files += $${ZCHX_OPENCV_PATH}/x64/mingw/bin/libopencv_imgproc420.dll
    }else{
        LIBS +=-L$${ZCHX_OPENCV_LIBS} \
                     -llibopencv_calib3d420 \
-llibopencv_core420 \
#-llibopencv_dnn420 \
#-llibopencv_features2d420 \
#-llibopencv_flann420 \
#-llibopencv_gapi420 \
#-llibopencv_highgui420 \
-llibopencv_imgcodecs420 \
-llibopencv_imgproc420 \
#-llibopencv_ml420 \
#-llibopencv_objdetect420 \
#-llibopencv_photo420 \
#-llibopencv_stitching420 \
#-llibopencv_video420 \
#-llibopencv_videoio420 \


        ZCHX_OPENCV_install.files += $${ZCHX_OPENCV_PATH}/x64/mingw/bin/libopencv_core420.dll
        ZCHX_OPENCV_install.files += $${ZCHX_OPENCV_PATH}/x64/mingw/bin/libopencv_imgcodecs420.dll
        ZCHX_OPENCV_install.files += $${ZCHX_OPENCV_PATH}/x64/mingw/bin/libopencv_imgproc420.dll
    }

    ZCHX_OPENCV_install.path = $${IDE_APP_PATH}/

    INSTALLS += ZCHX_OPENCV_install
}

!exists( $${ZCHX_OPENCV_PATH} ) {
    warning("Cann't find ZCHX_OPENCV DIR   =======================")
}

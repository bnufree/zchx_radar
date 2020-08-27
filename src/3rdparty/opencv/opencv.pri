
ZCHX_OPENCV_NAME        = opencv
ZCHX_OPENCV_VERSION     = 4.2.0
######################################################################
# 3rdparty path
######################################################################

#ZCHX_ECDIS_PATH = $${PSFW_3RDPARTYPATH}/$${ZCHX_ECDIS_NAME}/$${ZCHX_ECDIS_VERSION}
mingw{
    ZCHX_OPENCV_PATH = $${PWD}/$${ZCHX_OPENCV_VERSION}
}
unix{
    ZCHX_OPENCV_PATH = /usr/local
}

exists( $${ZCHX_OPENCV_PATH} ) {
message("find ZCHX_OPENCV DIR   =======================" ++ $$ZCHX_OPENCV_PATH)
    ######################################################################
    # Include library
    ######################################################################
mingw{
    ZCHX_OPENCV_HEADERS   = $${ZCHX_OPENCV_PATH}/include
    ZCHX_OPENCV_LIBS      = $${ZCHX_OPENCV_PATH}/x64/mingw/lib
}
unix{
    ZCHX_OPENCV_HEADERS   = $${ZCHX_OPENCV_PATH}/include/opencv4
    ZCHX_OPENCV_LIBS      = $${ZCHX_OPENCV_PATH}/lib64
}

    INCLUDEPATH += $${ZCHX_OPENCV_HEADERS}

mingw{
#    CONFIG(release, debug|release) {
#        LIBS +=-L$${ZCHX_OPENCV_LIBS} \
#                        -llibopencv_core420 \
#                        -llibopencv_imgcodecs420 \
#                        -llibopencv_imgproc420 \

#        ZCHX_OPENCV_install.files += $${ZCHX_OPENCV_PATH}/x64/mingw/bin/libopencv_core420.dll}
#        ZCHX_OPENCV_install.files += $${ZCHX_OPENCV_PATH}/x64/mingw/bin/libopencv_imgcodecs420.dll
#        ZCHX_OPENCV_install.files += $${ZCHX_OPENCV_PATH}/x64/mingw/bin/libopencv_imgproc420.dll
#    }
#    CONFIG(debug, debug|release){
#        LIBS +=-L$${ZCHX_OPENCV_LIBS} \
#                        -llibopencv_core420d \
#                        -llibopencv_imgcodecs420d \
#                        -llibopencv_imgproc420d \

#        ZCHX_OPENCV_install.files += $${ZCHX_OPENCV_PATH}/x64/mingw/bin/libopencv_core420d.dll
#        ZCHX_OPENCV_install.files += $${ZCHX_OPENCV_PATH}/x64/mingw/bin/libopencv_imgcodecs420d.dll
#        ZCHX_OPENCV_install.files += $${ZCHX_OPENCV_PATH}/x64/mingw/bin/libopencv_imgproc420d.dll
#    }

LIBS +=-L$${ZCHX_OPENCV_LIBS} \
                        -llibopencv_core420 \
                        -llibopencv_imgcodecs420 \
                        -llibopencv_imgproc420 \

        ZCHX_OPENCV_install.files += $${ZCHX_OPENCV_PATH}/x64/mingw/bin/libopencv_core420.dll}
        ZCHX_OPENCV_install.files += $${ZCHX_OPENCV_PATH}/x64/mingw/bin/libopencv_imgcodecs420.dll
        ZCHX_OPENCV_install.files += $${ZCHX_OPENCV_PATH}/x64/mingw/bin/libopencv_imgproc420.dll

    ZCHX_OPENCV_install.path = $${IDE_APP_PATH}/
    INSTALLS += ZCHX_OPENCV_install
}

unix{
        LIBS +=-L$${ZCHX_OPENCV_LIBS} \
                        -lopencv_core \
                        -lopencv_imgcodecs \
                        -lopencv_imgproc \
}

!exists( $${ZCHX_OPENCV_PATH} ) {
    warning("Cann't find ZCHX_OPENCV DIR   =======================")
}

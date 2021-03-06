######################################################################
# Automatically generated by qmake (2.01a) ?? 5? 13 01:01:53 2015
######################################################################

TEMPLATE = app
TARGET = 
DEPENDPATH += . Common FrameExtractor MFC_API
INCLUDEPATH += . MFC_API FrameExtractor Common

# Input
HEADERS += s3c_pp.h \
           widget.h \
           Common/lcd.h \
           Common/LogMsg.h \
           Common/mfc.h \
           Common/MfcDriver.h \
           Common/MfcDrvParams.h \
           Common/performance.h \
           Common/post.h \
           Common/videodev2.h \
           Common/videodev2_s3c.h \
           FrameExtractor/FileRead.h \
           FrameExtractor/FrameExtractor.h \
           FrameExtractor/H263Frames.h \
           FrameExtractor/H264Frames.h \
           FrameExtractor/MPEG4Frames.h \
           FrameExtractor/VC1Frames.h \
           MFC_API/SsbSipH264Decode.h \
           MFC_API/SsbSipH264Encode.h \
           MFC_API/SsbSipMfcDecode.h \
           MFC_API/SsbSipMpeg4Decode.h \
           MFC_API/SsbSipMpeg4Encode.h \
           MFC_API/SsbSipVC1Decode.h \
    treceiver.h \
    s3c-jpeg/JPGApi.h \
    s3c-jpeg/s3c_pp.h \
    rgb5652yuv422.h
FORMS += widget.ui
SOURCES += convert_rgb16_to_yuv420.c \
           main.cpp \
           widget.cpp \
           Common/LogMsg.c \
           Common/performance.c \
           FrameExtractor/FileRead.c \
           FrameExtractor/FrameExtractor.c \
           FrameExtractor/H263Frames.c \
           FrameExtractor/H264Frames.c \
           FrameExtractor/MPEG4Frames.c \
           FrameExtractor/VC1Frames.c \
           MFC_API/SsbSipH264Decode.c \
           MFC_API/SsbSipH264Encode.c \
           MFC_API/SsbSipMfcDecode.c \
           MFC_API/SsbSipMpeg4Decode.c \
           MFC_API/SsbSipMpeg4Encode.c \
           MFC_API/SsbSipVC1Decode.c \
    treceiver.cpp \
    s3c-jpeg/JPGApi.c

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/release/ -lrgb5652yuv422
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/debug/ -lrgb5652yuv422
else:unix: LIBS += -L$$PWD/ -lrgb5652yuv422

INCLUDEPATH += $$PWD/
DEPENDPATH += $$PWD/

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/release/librgb5652yuv422.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/debug/librgb5652yuv422.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/release/rgb5652yuv422.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/debug/rgb5652yuv422.lib
else:unix: PRE_TARGETDEPS += $$PWD/librgb5652yuv422.a

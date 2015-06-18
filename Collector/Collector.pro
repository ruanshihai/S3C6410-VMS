######################################################################
# Automatically generated by qmake (2.01a) ?? 6? 10 12:52:36 2015
######################################################################

TEMPLATE = app
TARGET = 
DEPENDPATH += . Common FrameExtractor MFC_API
INCLUDEPATH += . MFC_API FrameExtractor Common

# Input
HEADERS += s3c_pp.h \
           tcapture.h \
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
    widget.h
SOURCES += convert_rgb16_to_yuv420.c \
           main.cpp \
           tcapture.cpp \
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
    widget.cpp

FORMS += \
    widget.ui
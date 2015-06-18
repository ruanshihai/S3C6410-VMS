#include <QMessageBox>

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <time.h>
#include <jpeglib.h>

#include "widget.h"
#include "ui_widget.h"

#include "s3c-jpeg/JPGApi.h"
#include "Common/performance.h"
#include "rgb5652yuv422.h"

#define LOG_FILE_NAME "/tmp/jpeghw.log"
static int _debug = 1;
static void _log2file(const char* fmt, va_list vl)
{
    FILE* file_out;
    file_out = fopen(LOG_FILE_NAME,"a+");
    if (file_out == NULL) {
        return;
    }
    vfprintf(file_out, fmt, vl);
    fclose(file_out);
}
static void log2file(const char *fmt, ...)
{
    if (_debug) {
        va_list vl;
        va_start(vl, fmt);
        _log2file(fmt, vl);
        va_end(vl);
    }
}

//////////////////////////////////////////////////

struct _FBInfo;
typedef struct _FBInfo FBInfo;

struct _FBInfo
{
    int fd;
    unsigned char *bits;
    struct fb_fix_screeninfo fi;
    struct fb_var_screeninfo vi;
};

#define fb_width(fb)  ((fb)->vi.xres)
#define fb_height(fb) ((fb)->vi.yres)
#define fb_bpp(fb)    ((fb)->vi.bits_per_pixel>>3)
#define fb_size(fb)   ((fb)->vi.xres * (fb)->vi.yres * fb_bpp(fb))
#define FB_FILENAME "/dev/fb1"
#define PICTURN_PATH "/tmp/"

static int fb_open(FBInfo* fb, const char* fbfilename)
{
    fb->fd = open(fbfilename, O_RDWR);

    if (fb->fd < 0) {
        fprintf(stderr, "can't open %s\n", fbfilename);
        return -1;
    }

    if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->fi) < 0)
        goto fail;

    if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->vi) < 0)
        goto fail;

    fb->bits = (unsigned char*) mmap(0, fb_size(fb), PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);

    if (fb->bits == MAP_FAILED)
        goto fail;

    return 0;

fail:
    printf("%s is not a framebuffer.\n", fbfilename);
    close(fb->fd);

    return -1;
}

static void fb_close(FBInfo* fb)
{
    munmap(fb->bits, fb_size(fb));
    close(fb->fd);

    return;
}

void makeExifParam(ExifFileInfo *exifFileInfo)
{
    strcpy(exifFileInfo->Make,"Samsung SYS.LSI make");;
    strcpy(exifFileInfo->Model,"Samsung 2007 model");
    strcpy(exifFileInfo->Version,"version 1.0.2.0");
    strcpy(exifFileInfo->DateTime,"2007:05:16 12:32:54");
    strcpy(exifFileInfo->CopyRight,"Samsung Electronics@2007:All rights reserved");

    exifFileInfo->Height                    = 320;
    exifFileInfo->Width                     = 240;
    exifFileInfo->Orientation               = 1; // top-left
    exifFileInfo->ColorSpace                = 1;
    exifFileInfo->Process                   = 1;
    exifFileInfo->Flash                     = 0;
    exifFileInfo->FocalLengthNum            = 1;
    exifFileInfo->FocalLengthDen            = 4;
    exifFileInfo->ExposureTimeNum           = 1;
    exifFileInfo->ExposureTimeDen           = 20;
    exifFileInfo->FNumberNum                = 1;
    exifFileInfo->FNumberDen                = 35;
    exifFileInfo->ApertureFNumber           = 1;
    exifFileInfo->SubjectDistanceNum        = -20;
    exifFileInfo->SubjectDistanceDen        = -7;
    exifFileInfo->CCDWidth                  = 1;
    exifFileInfo->ExposureBiasNum           = -16;
    exifFileInfo->ExposureBiasDen           = -2;
    exifFileInfo->WhiteBalance              = 6;
    exifFileInfo->MeteringMode              = 3;
    exifFileInfo->ExposureProgram           = 1;
    exifFileInfo->ISOSpeedRatings[0]        = 1;
    exifFileInfo->ISOSpeedRatings[1]        = 2;
    exifFileInfo->FocalPlaneXResolutionNum  = 65;
    exifFileInfo->FocalPlaneXResolutionDen  = 66;
    exifFileInfo->FocalPlaneYResolutionNum  = 70;
    exifFileInfo->FocalPlaneYResolutionDen  = 71;
    exifFileInfo->FocalPlaneResolutionUnit  = 3;
    exifFileInfo->XResolutionNum            = 48;
    exifFileInfo->XResolutionDen            = 20;
    exifFileInfo->YResolutionNum            = 48;
    exifFileInfo->YResolutionDen            = 20;
    exifFileInfo->RUnit                     = 2;
    exifFileInfo->BrightnessNum             = -7;
    exifFileInfo->BrightnessDen             = 1;

    strcpy(exifFileInfo->UserComments,"Usercomments");
}

static int rgb5652jpeg(void *rgb565Buffer
    , int width
    , int height
    , char* outFilename)
{
    void    *InBuf;
    void    *OutBuf;
    long    jpegSize = 0;
    FILE*   fp;
    int     ret = 0;
    JPEG_ERRORTYPE result;
    ExifFileInfo    ExifInfo;
    long inBufferSize;

    log2file("rgb5652jpeg %d,%d\n", width, height);

    memset(&ExifInfo, 0x00, sizeof(ExifFileInfo));
    makeExifParam(&ExifInfo);
    ExifInfo.Width                     = width;
    ExifInfo.Height                    = height;

    int jpegEncodeHandle = SsbSipJPEGEncodeInit();
    if (jpegEncodeHandle < 0) {
        log2file("SsbSipJPEGDecodeInit failed!\n");
        return -1;
    }

    if (SsbSipJPEGSetConfig(JPEG_SET_SAMPING_MODE, JPG_422) != JPEG_OK) {
        log2file("SsbSipJPEGSetConfig failed!\n" );
        ret = -1;
        goto ENCODE_ERROR;
    }

    if (SsbSipJPEGSetConfig(JPEG_SET_ENCODE_WIDTH, width) != JPEG_OK) {
        log2file("SsbSipJPEGSetConfig failed!\n" );
        ret = -1;
        goto ENCODE_ERROR;
    }

    if (SsbSipJPEGSetConfig(JPEG_SET_ENCODE_HEIGHT, height) != JPEG_OK) {
        log2file("SsbSipJPEGSetConfig failed!\n");
        ret = -1;
        goto ENCODE_ERROR;
    }

    if (SsbSipJPEGSetConfig(JPEG_SET_ENCODE_QUALITY, JPG_QUALITY_LEVEL_1) != JPEG_OK) {
        log2file("SsbSipJPEGSetConfig failed!\n");
        ret = -1;
        goto ENCODE_ERROR;
    }

    inBufferSize = width*height*2;
    InBuf = SsbSipJPEGGetEncodeInBuf(jpegEncodeHandle, inBufferSize);
    if (InBuf == NULL) {
        log2file("SsbSipJPEGGetEncodeInBuf failed\n");
        ret = -1;
        goto ENCODE_ERROR;
    }

    Rgb565ToYuv422((unsigned char*)rgb565Buffer, width, height, (unsigned char*)InBuf);

    /*
    fp2 = fopen("/test_320_240.yuv", "rb");
    if(fp2 == NULL){
        ret = -1;
        goto ENCODE_ERROR;
    }
    fseek(fp2, 0, SEEK_END);
    fileSize = ftell(fp2);
    fseek(fp2, 0, SEEK_SET);
    log2file("inBufferSize = %ld, fileSize = %ld\n", inBufferSize, fileSize);
    fread(InBuf, 1, fileSize, fp2);
    fclose(fp2);
    */

    result = SsbSipJPEGEncodeExe(jpegEncodeHandle, &ExifInfo, JPEG_USE_HW_SCALER);
    if (result != JPEG_OK){
        log2file("SsbSipJPEGEncodeExe failed!\n");
        ret = -1;
        goto ENCODE_ERROR;
    }

    OutBuf = SsbSipJPEGGetEncodeOutBuf(jpegEncodeHandle, &jpegSize);
    if(OutBuf == NULL){
        log2file("SsbSipJPEGGetEncodeOutBuf failed!\n");
        ret = -1;
        goto ENCODE_ERROR;
    }

    fp = fopen(outFilename, "wb+");
    if(fp == NULL) {
        log2file("open %s file failed!\n", outFilename);
        ret = -1;
        goto ENCODE_ERROR;
    }
    fwrite(OutBuf, 1, jpegSize, fp);
    fclose(fp);

ENCODE_ERROR:
    SsbSipJPEGEncodeDeInit(jpegEncodeHandle);
    return ret;
}

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_SnapButton_clicked()
{
    FBInfo fb;
    int i,j;
    memset(&fb, 0x00, sizeof(fb));
    if (fb_open(&fb, FB_FILENAME) == 0) {
        char filename[255];
        char timeStr[255];
        time_t   lt;
        lt=time(NULL);
        strcpy(filename, PICTURN_PATH);
        strcpy(timeStr, asctime(localtime(&lt)));

        i = strlen(filename);
        for (j = 0; j<strlen(timeStr) && i<sizeof(filename); j++) {
            if (timeStr[j]>='0' && timeStr[j]<='9') {
                filename[i++] = timeStr[j];
            }
        }
        filename[i]='\0';
        strcat(filename, ".jpg");

        if ( rgb5652jpeg(fb.bits
                , fb_width(&fb)
                , fb_height(&fb)
                , filename) == 0) {
            //QMessageBox::information(this, "Snap", QString("Successed! The picturn has been save to %1.").arg(filename));

            /*
            QPixmap pixmap(filename);
            static QLabel* label = new QLabel(0);
            label->setPixmap(pixmap);
            label->showMaximized();
            */
        } else {
            QMessageBox::information(this, "Snap", "Error!");
        }
        fb_close(&fb);
    } else {
        QMessageBox::information(this, "Snap", "Error!");
    }
}

extern bool is_recording;
extern bool receive_enable;
extern FILE* encoded_fp;

void Widget::on_StopButton_clicked()
{
    is_recording = false;
    receive_enable = !receive_enable;
}

void Widget::on_RecordButton_clicked()
{
    if (!is_recording) {
        char filename[255];
        char timeStr[255];
        time_t   lt;
        lt=time(NULL);
        strcpy(filename, PICTURN_PATH);
        strcpy(timeStr, asctime(localtime(&lt)));

        int i = strlen(filename);
        for (int j = 0; j<strlen(timeStr) && i<sizeof(filename); j++) {
            if (timeStr[j]>='0' && timeStr[j]<='9') {
                filename[i++] = timeStr[j];
            }
        }
        filename[i]='\0';
        strcat(filename, ".h264");

        encoded_fp = fopen(filename, "wb+");
    } else {
        fclose(encoded_fp);
    }
    is_recording = !is_recording;
}

void Widget::on_PlaybackButton_clicked()
{

}

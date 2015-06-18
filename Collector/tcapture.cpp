#include <QtNetwork>

#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <linux/types.h>
#include <linux/fb.h>
#include <linux/videodev2.h>
#include <signal.h>

#include "MFC_API/SsbSipH264Encode.h"
#include "MFC_API/SsbSipH264Decode.h"
#include "MFC_API/SsbSipMpeg4Decode.h"
#include "MFC_API/SsbSipVC1Decode.h"
#include "FrameExtractor/FrameExtractor.h"
#include "FrameExtractor/MPEG4Frames.h"
#include "FrameExtractor/H263Frames.h"
#include "FrameExtractor/H264Frames.h"
#include "Common/LogMsg.h"
#include "Common/performance.h"
#include "Common/lcd.h"
#include "Common/MfcDriver.h"
#include "FrameExtractor/FileRead.h"

#include "s3c_pp.h"
#include "tcapture.h"

extern void convert_rgb16_to_yuv420(unsigned char *rgb, unsigned char *yuv, int width, int height);

#define MEDIA_FILE_NAME "/tmp/cam_encoding.264"
#define LCD_BPP_V4L2        V4L2_PIX_FMT_RGB565
#define VIDEO_WIDTH   320
#define VIDEO_HEIGHT  240
#define YUV_FRAME_BUFFER_SIZE   VIDEO_WIDTH*VIDEO_HEIGHT*2
#define PP_DEV_NAME     "/dev/s3c-pp"
extern int FriendlyARMWidth, FriendlyARMHeight;
#define FB0_BPP         16
#define FB0_COLOR_SPACE RGB16
#define VIDEO_FPS 30

/******************* CAMERA ********************/
class TError {
public:
    TError(const char *msg) {
        this->msg = msg;
    }
    TError(const TError &e) {
        msg = e.msg;
    }
    void Output() {
        std::cerr << msg << std::endl;
    }
    virtual ~TError() {}
protected:
    TError &operator=(const TError&);
private:
    const char *msg;
};

class TVideo {
public:
    TVideo(const char *DeviceName = "/dev/camera") {
        Width = VIDEO_WIDTH;
        Height = VIDEO_HEIGHT;
        BPP = 16;
        LineLen = Width * BPP / 8;
        Size = LineLen * Height;
        Addr = 0;
        fd = ::open(DeviceName, O_RDONLY);
        if (fd < 0) {
            TryAnotherCamera();
        }

        Addr = new unsigned char[Size];
        printf("Addr = %p, Size=%d\n", Addr, Size);
    }

    bool FetchPicture() const {
        int count = ::read(fd, Addr, Size);
        if (count != Size) {
            throw TError("error in fetching picture from video");
        }
        return true;
    }

    unsigned char *getAddr()
    {
        return Addr;
    }

    virtual ~TVideo() {
        ::close(fd);
        fd = -1;
        delete[] Addr;
        Addr = 0;
    }

protected:
    TVideo(const TVideo&);
    TVideo &operator=(const TVideo&);

private:
    int fd;
    unsigned char *Addr;
    int Size;
    int Width, Height, LineLen;
    unsigned BPP;
    void TryAnotherCamera();
};

class TFrameBuffer {
public:
    TFrameBuffer(const char *DeviceName = "/dev/fb1") {
        // LCD frame buffer open
        fb_fd = open(DeviceName, O_RDWR|O_NDELAY);
        if(fb_fd < 0)
        {
            printf("LCD frame buffer open error\n");
        }

        fb_size = VIDEO_WIDTH * VIDEO_HEIGHT * 2;	// RGB565
        fb_addr = (char *)mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
        if (fb_addr == NULL) {
            printf("LCD frame buffer mmap failed\n");
            return;
        }

        osd_info_to_driver.Bpp			= FB0_BPP;	// RGB16
        osd_info_to_driver.LeftTop_x	= 16;
        osd_info_to_driver.LeftTop_y	= 16;
        osd_info_to_driver.Width		= VIDEO_WIDTH;	// display width
        osd_info_to_driver.Height		= VIDEO_HEIGHT;	// display height

        // set OSD's information
        if(ioctl(fb_fd, SET_OSD_INFO, &osd_info_to_driver)) {
            printf("Some problem with the ioctl SET_OSD_INFO\n");
            return;
        }

        ioctl(fb_fd, SET_OSD_START);
    }

    bool DrawRect(TVideo &SrcVideo) const {
        memcpy(fb_addr, SrcVideo.getAddr(), fb_size);			// LCD frame buffer

        return true;
    }

    virtual ~TFrameBuffer() {
        ioctl(fb_fd, SET_OSD_STOP);
        munmap(fb_addr, fb_size);

        ::close(fb_fd);
        fb_fd = -1;
    }

protected:
    TFrameBuffer(const TFrameBuffer&);
    TFrameBuffer &operator=( const TFrameBuffer&);
private:
    int          fb_size;
    int          fb_fd;
    char         *fb_addr;
    s3c_win_info_t osd_info_to_driver;
};

/* MFC functions */
static void *mfc_encoder_init(int width, int height, int frame_rate, int bitrate, int gop_num);
static void *mfc_encoder_exe(void *handle, unsigned char *yuv_buf, int frame_size, int first_frame, long *size);
static void mfc_encoder_free(void *handle);

class TH264Encoder {
public:
    TH264Encoder() {
        frame_count = 0;
        handle = mfc_encoder_init(VIDEO_WIDTH, VIDEO_HEIGHT, 15, 1000, 15);
        if (handle == 0) {
            throw TError("cannot init mfc encoder");
        }
        encoded_fp = fopen(MEDIA_FILE_NAME, "wb+");
        if (encoded_fp == 0) {
            throw TError("cannot open /tmp/cam_encoding.264");
        }

        sender = new QUdpSocket();
        serverAddress = QHostAddress("192.168.1.168");
    }

    virtual ~TH264Encoder() {
        mfc_encoder_free(handle);
        fclose(encoded_fp);
    }

    void Encode(TVideo &rect)
    {
        frame_count++;
        unsigned char* pRgbData = rect.getAddr();

        convert_rgb16_to_yuv420(pRgbData, g_yuv, VIDEO_WIDTH, VIDEO_HEIGHT);

        //Rgb565ToYuv422(pRgbData, VIDEO_WIDTH, VIDEO_HEIGHT, g_yuv);

        if(frame_count == 1)
            encoded_buf = (unsigned char*)mfc_encoder_exe(handle, g_yuv, YUV_FRAME_BUFFER_SIZE, 1, &encoded_size);
        else
            encoded_buf = (unsigned char*)mfc_encoder_exe(handle, g_yuv, YUV_FRAME_BUFFER_SIZE, 0, &encoded_size);

        //printf("Send frame %d\n", frame_count);
        sender->writeDatagram((char *)encoded_buf, encoded_size, serverAddress, 6665);

        //fwrite(encoded_buf, 1, encoded_size, encoded_fp);
    }

protected:
    TH264Encoder(const TH264Encoder&);
    TH264Encoder &operator=( const TH264Encoder&);
private:
    int frame_count;
    void* handle;
    FILE* encoded_fp;

    unsigned char   g_yuv[YUV_FRAME_BUFFER_SIZE];
    unsigned char   *encoded_buf;
    long            encoded_size;

    QUdpSocket *sender;
    QHostAddress serverAddress;
};

void TVideo::TryAnotherCamera()
{
    int ret, start, found;

    struct v4l2_input chan;
    struct v4l2_framebuffer preview;

    fd = ::open("/dev/video0", O_RDWR);
    if (fd < 0) {
        throw TError("cannot open video device");
    }

    /* Get capability */
    struct v4l2_capability cap;
    ret = ::ioctl(fd , VIDIOC_QUERYCAP, &cap);
    if (ret < 0) {
        throw TError("not available device");
    }

    /* Check the type - preview(OVERLAY) */
    if (!(cap.capabilities & V4L2_CAP_VIDEO_OVERLAY)) {
        throw TError("not available device");
    }
    chan.index = 0;
    found = 0;
    while(1) {
        ret = ::ioctl(fd, VIDIOC_ENUMINPUT, &chan);
        if (ret < 0) {
            throw TError("not available device");
        }

        if (chan.type &V4L2_INPUT_TYPE_CAMERA ) {
            found = 1;
            break;
        }
        chan.index++;
    }

    if (!found) {
        throw TError("not available device");
    }

    chan.type = V4L2_INPUT_TYPE_CAMERA;
    ret = ::ioctl(fd, VIDIOC_S_INPUT, &chan);
    if (ret < 0) {
        throw TError("not available device");
    }

    memset(&preview, 0, sizeof preview);
    preview.fmt.width = Width;
    preview.fmt.height = Height;
    preview.fmt.pixelformat = V4L2_PIX_FMT_RGB565;
    preview.capability = 0;
    preview.flags = 0;

    /* Set up for preview */
    ret = ioctl(fd, VIDIOC_S_FBUF, &preview);
    if (ret< 0) {
        throw TError("not available device");
    }

    /* Preview start */
    start = 1;
    ret = ioctl(fd, VIDIOC_OVERLAY, &start);
    if (ret < 0) {
        throw TError("not available device");
    }
}

/***************** MFC driver function *****************/
void *mfc_encoder_init(int width, int height, int frame_rate, int bitrate, int gop_num)
{
    int             frame_size;
    void            *handle;
    int             ret;

    frame_size  = (width * height * 3) >> 1;

    handle = SsbSipH264EncodeInit(width, height, frame_rate, bitrate, gop_num);
    if (handle == NULL) {
        LOG_MSG(LOG_ERROR, "Test_Encoder", "SsbSipH264EncodeInit Failed\n");
        return NULL;
    }

    ret = SsbSipH264EncodeExe(handle);

    return handle;
}

void *mfc_encoder_exe(void *handle, unsigned char *yuv_buf, int frame_size, int first_frame, long *size)
{
    unsigned char   *p_inbuf, *p_outbuf;
    int             hdr_size;
    int             ret;


    p_inbuf = (unsigned char*)SsbSipH264EncodeGetInBuf(handle, 0);

    memcpy(p_inbuf, yuv_buf, frame_size);

    ret = SsbSipH264EncodeExe(handle);
    if (first_frame) {
        SsbSipH264EncodeGetConfig(handle, H264_ENC_GETCONF_HEADER_SIZE, &hdr_size);
        //printf("Header Size : %d\n", hdr_size);
    }

    p_outbuf = (unsigned char*)SsbSipH264EncodeGetOutBuf(handle, size);

    return p_outbuf;
}

void mfc_encoder_free(void *handle)
{
    SsbSipH264EncodeDeInit(handle);
}


#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>

int FriendlyARMWidth, FriendlyARMHeight;
static void FBOpen(void)
{
    struct fb_fix_screeninfo FBFix;
    struct fb_var_screeninfo FBVar;
    int FBHandle = -1;

    FBHandle = open("/dev/fb0", O_RDWR);
    if (ioctl(FBHandle, FBIOGET_FSCREENINFO, &FBFix) == -1 ||
        ioctl(FBHandle, FBIOGET_VSCREENINFO, &FBVar) == -1) {
        fprintf(stderr, "Cannot get Frame Buffer information");
        exit(1);
    }

    FriendlyARMWidth  = FBVar.xres;
    FriendlyARMHeight = FBVar.yres;
    close(FBHandle);
}

void TCapture::run() {
    FBOpen();
    try {
        struct timeval start,end;
        TFrameBuffer FrameBuffer;
        TVideo Video;
        TH264Encoder Encoder;
        int timeuse = 0;
        int fms = 1000 / VIDEO_FPS;

        extern bool is_thread_running;
        //for (;;) {
        while (is_thread_running) {
            gettimeofday( &start, NULL );

            Video.FetchPicture();
            Encoder.Encode(Video);
            FrameBuffer.DrawRect(Video);

            gettimeofday( &end, NULL );
            timeuse = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;
            timeuse /= 1000;
            if (timeuse < fms)
                msleep(fms-timeuse);
        }

        printf("\nDone!\n");

    } catch (TError &e) {
        e.Output();
        return;
    }
}

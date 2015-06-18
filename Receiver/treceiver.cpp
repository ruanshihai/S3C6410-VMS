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

#include "Common/LogMsg.h"
#include "Common/performance.h"
#include "Common/lcd.h"
#include "Common/MfcDriver.h"

#include "s3c_pp.h"
#include "treceiver.h"

#define MEDIA_FILE_NAME "/tmp/cam_encoding.264"
#define LCD_BPP_V4L2        V4L2_PIX_FMT_RGB565
#define VIDEO_WIDTH   320
#define VIDEO_HEIGHT  240
#define YUV_FRAME_BUFFER_SIZE   VIDEO_WIDTH*VIDEO_HEIGHT*2
#define PP_DEV_NAME     "/dev/s3c-pp"
extern int FriendlyARMWidth, FriendlyARMHeight;
//#define FB0_WIDTH FriendlyARMWidth
//#define FB0_HEIGHT FriendlyARMHeight
#define FB0_WIDTH 320
#define FB0_HEIGHT 240
#define FB0_BPP         16
#define FB0_COLOR_SPACE RGB16

class TH264Decoder {
public:
    TH264Decoder() {
        // Post processor open
        pp_fd = open(PP_DEV_NAME, O_RDWR);
        if(pp_fd < 0)
        {
            printf("Post processor open error\n");
        }

        // LCD frame buffer open
        fb_fd = open("/dev/fb1", O_RDWR|O_NDELAY);
        if(fb_fd < 0)
        {
            printf("LCD frame buffer open error\n");
        }

        //////////////////////////////////////
        ///    1. Create new instance      ///
        ///      (SsbSipH264DecodeInit)    ///
        //////////////////////////////////////
        handle = SsbSipH264DecodeInit();
        if (handle == NULL) {
            printf("H264_Dec_Init Failed.\n");
        }
    }

    virtual ~TH264Decoder() {
        ioctl(fb_fd, SET_OSD_STOP);
        SsbSipH264DecodeDeInit(handle);

        munmap(fb_addr, fb_size);
        close(pp_fd);
        close(fb_fd);
    }

    int Decode(char *encoded_buf, int encoded_size, bool *first_frame)
    {
        if (*first_frame) {
            *first_frame = false;

            /////////////////////////////////////////////
            ///    2. Obtaining the Input Buffer      ///
            ///      (SsbSipH264DecodeGetInBuf)       ///
            /////////////////////////////////////////////
            pStrmBuf = SsbSipH264DecodeGetInBuf(handle, 0);
            if (pStrmBuf == NULL) {
                printf("SsbSipH264DecodeGetInBuf Failed.\n");
                SsbSipH264DecodeDeInit(handle);
            }
            memcpy(pStrmBuf, encoded_buf, encoded_size);

            ////////////////////////////////////////////////////////////////
            ///    3. Configuring the instance with the config stream    ///
            ///       (SsbSipH264DecodeExe)                             ///
            ////////////////////////////////////////////////////////////////
            if (SsbSipH264DecodeExe(handle, encoded_size) != SSBSIP_H264_DEC_RET_OK) {
                printf("H.264 Decoder Configuration Failed.\n");
                return -1;
            }

            /////////////////////////////////////
            ///   4. Get stream information   ///
            /////////////////////////////////////
            SsbSipH264DecodeGetConfig(handle, H264_DEC_GETCONF_STREAMINFO, &stream_info);

            //printf("\t<STREAMINFO> width=%d   height=%d.\n", stream_info.width, stream_info.height);

            // set post processor configuration
            pp_param.src_full_width	    = stream_info.buf_width;
            pp_param.src_full_height	= stream_info.buf_height;
            pp_param.src_start_x		= 0;
            pp_param.src_start_y		= 0;
            pp_param.src_width			= pp_param.src_full_width;
            pp_param.src_height			= pp_param.src_full_height;
            pp_param.src_color_space	= YC420;
            pp_param.dst_start_x		= 0;
            pp_param.dst_start_y		= 0;
            pp_param.dst_full_width	    = FB0_WIDTH;		// destination width
            pp_param.dst_full_height	= FB0_HEIGHT;		// destination height
            pp_param.dst_width			= pp_param.dst_full_width;
            pp_param.dst_height			= pp_param.dst_full_height;
            pp_param.dst_color_space	= FB0_COLOR_SPACE;
            pp_param.out_path           = DMA_ONESHOT;

            ioctl(pp_fd, S3C_PP_SET_PARAMS, &pp_param);

            // get LCD frame buffer address
            fb_size = pp_param.dst_full_width * pp_param.dst_full_height * 2;	// RGB565
            fb_addr = (char *)mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
            if (fb_addr == NULL) {
                printf("LCD frame buffer mmap failed\n");
                return -1;
            }

            osd_info_to_driver.Bpp			= FB0_BPP;	// RGB16
            osd_info_to_driver.LeftTop_x	= 16;
            osd_info_to_driver.LeftTop_y	= 16;
            osd_info_to_driver.Width		= FB0_WIDTH;	// display width
            osd_info_to_driver.Height		= FB0_HEIGHT;	// display height

            // set OSD's information
            if(ioctl(fb_fd, SET_OSD_INFO, &osd_info_to_driver)) {
                printf("Some problem with the ioctl SET_OSD_INFO\n");
                return -1;
            }

            ioctl(fb_fd, SET_OSD_START);
        }

        //////////////////////////////////
        ///       5. DECODE            ///
        ///    (SsbSipH264DecodeExe)   ///
        //////////////////////////////////
        memcpy(pStrmBuf, encoded_buf, encoded_size);
        if (SsbSipH264DecodeExe(handle, encoded_size) != SSBSIP_H264_DEC_RET_OK)
            return -1;

        //////////////////////////////////////////////
        ///    6. Obtaining the Output Buffer      ///
        ///      (SsbSipH264DecodeGetOutBuf)       ///
        //////////////////////////////////////////////
        SsbSipH264DecodeGetConfig(handle, H264_DEC_GETCONF_PHYADDR_FRAM_BUF, pYUVBuf);

        // Post processing
        pp_param.src_buf_addr_phy		= pYUVBuf[0];	// MFC output buffer
        ioctl(pp_fd, S3C_PP_SET_SRC_BUF_ADDR_PHY, &pp_param);

        ioctl(fb_fd, FBIOGET_FSCREENINFO, &lcd_info);
        pp_param.dst_buf_addr_phy		= lcd_info.smem_start;			// LCD frame buffer
        ioctl(pp_fd, S3C_PP_SET_DST_BUF_ADDR_PHY, &pp_param);
        ioctl(pp_fd, S3C_PP_START);

        return 0;
    }

protected:
    TH264Decoder(const TH264Decoder&);
    TH264Decoder &operator=( const TH264Decoder&);
private:
    void*        handle;
    void         *pStrmBuf;
    int          fb_size;
    int          pp_fd, fb_fd;
    char         *fb_addr;
    unsigned int	pYUVBuf[2];

    s3c_pp_params_t	pp_param;
    s3c_win_info_t osd_info_to_driver;
    SSBSIP_H264_STREAM_INFO stream_info;
    struct fb_fix_screeninfo lcd_info;
};


TReceiver::TReceiver()
{
}

extern bool is_recording;
extern bool receive_enable;
extern FILE* encoded_fp;

void TReceiver::run()
{
    first_frame = true;
    Decoder = new TH264Decoder();

    receiver = new QUdpSocket();
    receiver->bind(QHostAddress("192.168.1.168"), 6665);

    //connect(receiver, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));

    QByteArray datagram;
    datagram.resize(65536);

    int count = 0;
    printf("Receiver thread is runnning...\n");
    while (receiver->waitForReadyRead()) {
        receiver->readDatagram(datagram.data(), datagram.size());
        printf("Receive frame (%d)\n", ++count);

        if (receive_enable) {
            Decoder->Decode(datagram.data(), datagram.size(), &first_frame);

            if (is_recording && encoded_fp) {
                fwrite(datagram.data(), 1, datagram.size(), encoded_fp);
            }
        }
    }
}

void TReceiver::readPendingDatagrams()
{
    while (receiver->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(receiver->pendingDatagramSize());
        receiver->readDatagram(datagram.data(), datagram.size());
        Decoder->Decode(datagram.data(), datagram.size(), &first_frame);
    }
}

//
// create by 李雨晴 on 2018/9/27.
//

#ifndef LPUSHER_VIDEOCHANNEL_H
#define LPUSHER_VIDEOCHANNEL_H


#include <inttypes.h>
#include <x264.h>
#include <pthread.h>
#include "librtmp/rtmp.h"

class VideoChannel{

    typedef void (*VideoCallBack)(RTMPPacket* packet);
public:
    VideoChannel();
    ~VideoChannel();

    //创建x264的编码器
    void setVideoEncInfo(int width,int height,int fps,int bitrate);

    void encodeData(int8_t *data);

    void setVideoCallBack(VideoCallBack videoCallBack);


private:
    pthread_mutex_t mutex;
    int mWidth;
    int mHeight;
    int mFps;
    int mBitrate;

    x264_t *videoCodec =0;
    x264_picture_t *pic_in = 0;

    int ySize;
    int uvSize;
    int index = 0;

    VideoCallBack  videoCallBack;

    void sendSpsPps(uint8_t *sps,uint8_t *pps,int sps_len,int pps_len);
    void sendFrame(int type,int payload,uint8_t *p_payload);
};




#endif //LPUSHER_VIDEOCHANNEL_H

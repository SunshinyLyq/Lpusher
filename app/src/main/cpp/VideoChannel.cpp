//
// create by 李雨晴 on 2018/9/27.
//


#include <stdint.h>
#include <x264.h>
#include "VideoChannel.h"
#include "macro.h"

VideoChannel::VideoChannel() {
    pthread_mutex_init(&mutex,0);

}


VideoChannel::~VideoChannel() {
    pthread_mutex_destroy(&mutex);

    if (videoCodec){
        x264_encoder_close(videoCodec);
        videoCodec = 0;
    }

    if (pic_in){
        x264_picture_clean(pic_in);
        DELETE(pic_in)
    }


}

/**
 * 设置编码器相关信息
 * @param width
 * @param height
 * @param fps
 * @param bitrate
 */
void VideoChannel::setVideoEncInfo(int width, int height, int fps, int bitrate) {
    //推流过程中可能会改变大小，从而会使用到这个编码器
    //多线程
    //一个线程可能在进行编码推流，另一个线程可能在改变这个编码器
    pthread_mutex_lock(&mutex);

    mWidth=width;
    mHeight=height;
    mFps=fps;
    mBitrate=bitrate;

    ySize = width * height;
    uvSize = ySize / 4;

    if (videoCodec){
        x264_encoder_close(videoCodec);
        videoCodec = 0;
    }

    if (pic_in){
        x264_picture_clean(pic_in);
        delete pic_in;
        pic_in = 0 ;
    }

    //打开x264编码器
    //x264的编码器属性
    x264_param_t param;
    //第2个参数  最快，第3个参数 无延迟编码
    x264_param_default_preset(&param,"ultrafast","zerolatency");
    //编码规格
    param.i_level_idc = 32;
    //输入数据格式
    param.i_csp = X264_CSP_I420;
    param.i_width = width;
    param.i_height = height;
    //无b帧
    param.i_bframe = 0;
    //参数rc.i_rc_method 表示码率控制，CQR(恒定质量)，CRF（恒定码率）,ABR(平均码率)
    param.rc.i_rc_method = X264_RC_ABR;
    //码率（单位kps）
    param.rc.i_bitrate = bitrate / 1000 ;
    //瞬时最大码率
    param.rc.i_vbv_max_bitrate = bitrate / 1000 * 1.2;
    //码率控制区大小,设置了i_vbv_max_bitrate必须设置此参数
    param.rc.i_vbv_buffer_size = bitrate / 1000 ;


    //帧率
    param.i_fps_num = fps; //分子
    param.i_fps_den = 1;//分母
    param.i_timebase_den = param.i_fps_num ; //时间戳的分母
    param.i_timebase_num = param.i_fps_den; //时间戳的分子
    //用fps而不是时间戳来计算帧间距离
    param.b_vfr_input =0;
    //帧距离 (关键帧) 2s一个关键帧
    param.i_keyint_max = fps * 2;
    //是否复制sps和pps放在每个关键帧的前面
    //该参数设置是让每个关键帧（I帧）都附带sps和pps
    param.b_repeat_headers = 1;
    //多线程
    param.i_threads = 1;

    x264_param_apply_profile(&param,"baseline");

    //打开编码器
    videoCodec=x264_encoder_open(&param);
    //x264_picture_t结构体，描述视频帧的
    pic_in=new x264_picture_t;
    x264_picture_alloc(pic_in,X264_CSP_I420,width,height);

    pthread_mutex_unlock(&mutex);
}

/**
 * 编码数据
 * @param data
 */
void VideoChannel::encodeData(int8_t *data) {
    //编码
    pthread_mutex_lock(&mutex);

    //将数据存放在pic_in中，
    //传递过来的数据格式为NV21，而X264需要输入的数据格式为I420，所以需要转换一下
    //y数据
    //plane平面，YUV代表有3个平面，0就代表y,1就代表u,2就代表v
    //y数据没有变化，所以直接拷贝过来就可以了
    memcpy(pic_in->img.plane[0],data,ySize);

    //uv数据
    for (int i = 0; i < uvSize; ++i) {
        //间隔1个字节取一个数据
        //u数据
        *(pic_in->img.plane[1] + i) = *(data + ySize + i * 2 +1);
        //v数据
        *(pic_in->img.plane[2] + i) = * (data + ySize + i * 2);
    }

    pic_in->i_pts = index++;

    //编码出的数据
    x264_nal_t *pp_nal;
    //编码出了几个nalu(暂时理解为帧)
    int pi_nal;
    x264_picture_t pic_out;
    //编码
    int ret = x264_encoder_encode(videoCodec,&pp_nal,&pi_nal,pic_in,&pic_out);
    //编码出的数据格式为h264数据
    if (ret < 0){
        pthread_mutex_unlock(&mutex);
        return;
    }

    int sps_len,pps_len;
    uint8_t sps[100];
    uint8_t pps[100];

    for (int i = 0; i < pi_nal; ++i) {
        //数据类型
        if (pp_nal[i].i_type == NAL_SPS){
            //去掉00 00 00 01
            //加入到RTMPPacket的时候，是去掉h264分割符的
            sps_len = pp_nal[i].i_payload - 4;//数据大小
            memcpy(sps,pp_nal[i].p_payload + 4,sps_len); //数据内容

        }else if(pp_nal[i].i_type == NAL_PPS){
            pps_len = pp_nal[i].i_payload - 4;
            memcpy(pps,pp_nal[i].p_payload + 4,pps_len);

            //拿到pps ，就表示sps也拿到了
            //发送
            sendSpsPps(sps,pps,sps_len,pps_len);
        }else{
            //关键帧，非关键帧
            sendFrame(pp_nal[i].i_type,pp_nal[i].i_payload,pp_nal[i].p_payload);
        }
    }


    pthread_mutex_unlock(&mutex);
}

void VideoChannel::sendSpsPps(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len) {
    RTMPPacket *packet = new RTMPPacket;

    int bodysize=13 + sps_len + 3 + pps_len ;
    RTMPPacket_Alloc(packet,bodysize);

    int i=0;

    //固定头：
    packet->m_body[i++] = 0x17;
    //类型
    packet->m_body[i++] = 0x00;
    //composition time
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;

    //版本：
    packet->m_body[i++] = 0x01;
    //编码规格
    packet->m_body[i++] = sps[1];
    packet->m_body[i++] = sps[2];
    packet->m_body[i++] = sps[3];
    packet->m_body[i++] = 0xFF;
    //整个sps
    packet->m_body[i++] = 0xE1;

    //sps长度
    packet->m_body[i++] = (sps_len >> 8) & 0xff ;
    packet->m_body[i++] = sps_len & 0xff ;
    memcpy(&packet->m_body[i],sps,sps_len);
    i += sps_len;

    //pps
    packet->m_body[i++] = 0x01;
    packet->m_body[i++] = (pps_len >> 8) & 0xff;
    packet->m_body[i++] = pps_len & 0xff;
    memcpy(&packet->m_body[i],pps,pps_len);

    //视频
    packet->m_packetType =RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = bodysize;
    //随意分配一个管道（尽量避开rtmp.c中使用的）
    packet->m_nChannel =10;
    //sps pps没有时间戳
    packet->m_nTimeStamp = 0;
    //不适用绝对时间
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM ;
    videoCallBack(packet);
}

void VideoChannel::setVideoCallBack(VideoCallBack callBack) {
    this->videoCallBack=callBack;
}

void VideoChannel::sendFrame(int type, int payload, uint8_t *p_payload) {
    if(p_payload[2] == 0x00){
        payload -= 4;
        p_payload += 4;
    }else if (p_payload[2] == 0x01){
        payload -= 3;
        p_payload +=3;
    }

    RTMPPacket* packet= new RTMPPacket;
    int body_size = 9 + payload;

    RTMPPacket_Alloc(packet,body_size);
    RTMPPacket_Reset(packet);

    packet->m_body[0] = 0x27;
    if (type == NAL_SLICE_IDR){
        //关键帧
        packet->m_body[0] = 0x17;
    }

    //类型
    packet->m_body[1] = 0x01;
    //时间戳
    packet->m_body[2]= 0x00;
    packet->m_body[3]= 0x00;
    packet->m_body[4] = 0x00;

    //数据长度 int 4个字节，相当于把int转成4个字节的byte数组
    packet->m_body[5] = (payload >> 24) & 0xff;
    packet->m_body[6] = (payload >> 16) & 0xff;
    packet->m_body[7] = (payload >> 8) & 0xff;
    packet->m_body[8] = (payload) & 0xff;

    //图片数据
    memcpy(&packet->m_body[9],p_payload,payload);

    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize = body_size;
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nChannel = 0x10;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    videoCallBack(packet);
}















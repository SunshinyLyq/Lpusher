//
// create by 李雨晴 on 2018/9/29.
//



#include <cstdint>
#include <sys/types.h>
#include <cstring>
#include "AudioChannel.h"
#include "macro.h"
#include "librtmp/rtmp.h"

AudioChannel ::AudioChannel() {

}

AudioChannel::~AudioChannel() {

    DELETE(buffer);
    //释放编码器
    if (audioCodec){
        faacEncClose(audioCodec);
        audioCodec = 0;
    }
}


int AudioChannel::getInputSamples() {

    return inputSamples;
}


void AudioChannel::setAudioEncInfo(int sameplesInHz, int channels) {

    //打开编码器
    mChannels = channels;

    //3.一次最大能输入编码器的样本数量，也就是编码的数据的个数（一个样本是16位，2个字节）
    //4.最大可能的输出的数据，也就是编码后的最大字节数
    audioCodec = faacEncOpen(sameplesInHz,channels,&inputSamples,&maxOutputBytes);

    //设置编码器参数
    faacEncConfigurationPtr  config = faacEncGetCurrentConfiguration(audioCodec);
    //指定为 mpeg4 标准
    config->mpegVersion = MPEG4;
    //lc 标准
    config->aacObjectType = LOW;
    //16位
    config->inputFormat = FAAC_INPUT_16BIT;
    //编码出的原始数据  既不是adts也不是adif
    config->outputFormat = 0;
    faacEncSetConfiguration(audioCodec,config);

    //输出缓冲区 编码后的数据用这个缓冲区保存
    buffer = new u_char[maxOutputBytes];
}


RTMPPacket* AudioChannel::getAudioTag() {
    u_char  *buf;
    u_long len;
    faacEncGetDecoderSpecificInfo(audioCodec,&buf,&len);
    int bodySize = 2 + len;
    RTMPPacket *packet = new RTMPPacket;
    RTMPPacket_Alloc(packet,bodySize);

    //双声道
    packet->m_body[0] = 0xAF;
    if (mChannels == 1){
        packet->m_body[0] = 0xAE;
    }

    packet->m_body[1] =0x00;
    memcpy(&packet->m_body[2],buf,len);

    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize = bodySize;
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nChannel = 0x11;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;


    return packet;
}

void AudioChannel::encodeData(int8_t *data) {
    //返回编码后数据字节的长度
    int bytelen = faacEncEncode(audioCodec, reinterpret_cast<int32_t *>(data), inputSamples, buffer, maxOutputBytes);

    if (bytelen > 0){
        int bosySize = 2 + bytelen;
        RTMPPacket *packet=new RTMPPacket;
        RTMPPacket_Alloc(packet,bosySize);

        //双声道
        packet->m_body[0] = 0xAF;
        if (mChannels == 1){
            packet->m_body[0] = 0xAE;
        }

        //编码出的声音都是 0x01
        packet->m_body[1] = 0x01;

        //图片数据
        memcpy(&packet->m_body[2],buffer,bytelen);

        packet->m_hasAbsTimestamp = 0;
        packet->m_nBodySize = bosySize;
        packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
        packet->m_nChannel = 0x11;
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
        audioCallback(packet);
    }

}

void AudioChannel::setAudioCallback(AudioCallback audioCallback) {
    this->audioCallback=audioCallback;
}
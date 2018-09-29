//
// create by 李雨晴 on 2018/9/29.
//

#ifndef LPUSHER_AUDIOCHANNEL_H
#define LPUSHER_AUDIOCHANNEL_H

#include "faac.h"
#include "librtmp/rtmp.h"


class AudioChannel{

    typedef void (*AudioCallback)(RTMPPacket *packet);
public:
    AudioChannel();
    ~AudioChannel();

    void setAudioEncInfo(int sameplesInHz,int channels);
    void setAudioCallback(AudioCallback audioCallback);

    int getInputSamples();
    void encodeData(int8_t *data);

    RTMPPacket *getAudioTag();

private:
    AudioCallback  audioCallback;
    int mChannels;
    faacEncHandle  audioCodec = 0;

    unsigned long inputSamples;
    unsigned long maxOutputBytes;

    u_char *buffer = 0;


};
#endif //LPUSHER_AUDIOCHANNEL_H

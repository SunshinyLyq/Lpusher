#include <jni.h>
#include "librtmp/rtmp.h"
#include "safe_queue.h"
#include "macro.h"
#include "VideoChannel.h"
#include "AudioChannel.h"

SafeQueue<RTMPPacket *> packets;
VideoChannel *videoChannel = 0;
int isStart = 0;
pthread_t pid;

int readyPushing = 0;
uint32_t start_time;

AudioChannel *audioChannel = 0;

void releasePackets(RTMPPacket *&packet) {
    if (packet) {
        RTMPPacket_Free(packet);
        DELETE(packet);
    }
}


void callBack(RTMPPacket *packet) {
    if (packet) {
        packet->m_nTimeStamp = RTMP_GetTime() - start_time;
        //将数据塞到队列中
        packets.push(packet);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_lyq_com_lpusher_live_LivePusher_native_1init(JNIEnv *env, jobject instance) {

    // TODO 初始化

    //准备一个Video编码器的工具类：进行编码
    videoChannel = new VideoChannel;
    //audio编码器工具类
    audioChannel = new AudioChannel;

    videoChannel->setVideoCallBack(callBack);
    audioChannel->setAudioCallback(callBack);

    //准备一个队列，打包好的数据放入队列，在线程中统一的取出数据再发送给服务器
    packets.setReleaseCallback(releasePackets);

}



extern "C"
JNIEXPORT void JNICALL
Java_lyq_com_lpusher_live_LivePusher_native_1setVideoEncInfo(JNIEnv *env, jobject instance,
                                                             jint width, jint height, jint fps,
                                                             jint bitrate) {

    // 初始化编码器，当推流大小改变时，相应的编码器的内容也需要改变
    if (videoChannel) {
        videoChannel->setVideoEncInfo(width, height, fps, bitrate);
    }

}


void *start(void *args) {

    char *url = static_cast<char *>(args);
    //连接rtmp服务器
    RTMP *rtmp = 0;
    do {
        //申请内存
        rtmp = RTMP_Alloc();
        if (!rtmp) {
            LOGE("rtmp创建失败");
            break;
        }
        //初始化
        RTMP_Init(rtmp);
        //设置超时时间 5s
        /**
         * 这个超时时间无效，其实这个是接受服务器响应的超时时间，不包括我们连接服务器，我们发送数据的超时时间
         * 去源码修改一下
         */
        rtmp->Link.timeout = 5;
        //设置地址
        int ret = RTMP_SetupURL(rtmp, url);
        if (!ret) {
            LOGE("rtmp设置地址失败：%s", url);
            break;
        }

        //开启输出模式
        RTMP_EnableWrite(rtmp);
        //连接服务器
        ret = RTMP_Connect(rtmp, 0);
        if (!ret) {
            LOGE("rtmp连接地址失败 ： %s", url);
            break;
        }

        //连接流
        ret = RTMP_ConnectStream(rtmp, 0);
        if (!ret) {
            LOGE("rtmp连接流失败：%s", url);
            break;
        }

        //准备好了，可以开始推流
        readyPushing = 1;
        //记录一个开始推流的时间
        start_time = RTMP_GetTime();
        packets.setWork(1);

        //保证第一个数据是 aac解码数据包
        callBack(audioChannel->getAudioTag());
        RTMPPacket *packet = 0;
        //循环从队列中取出数据包，然后发送
        while (isStart) {
            packets.pop(packet);

            if (!isStart) {
                break;
            }

            if (!packet) {
                continue;
            }
            //给rtmp的流id
            packet->m_nInfoField2 = rtmp->m_stream_id;
            //发送数据包 1表示队列
            ret = RTMP_SendPacket(rtmp, packet, 1);
            releasePackets(packet);

            if (!ret) {
                LOGE("发送数据失败");
                break;
            }

        }
        releasePackets(packet);
    } while (0);

    packets.setWork(0);
    packets.clear();

    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }

    delete url;
    return 0;

}


extern "C"
JNIEXPORT void JNICALL
Java_lyq_com_lpusher_live_LivePusher_native_1start(JNIEnv *env, jobject instance, jstring path_) {
    // 开始推流
    if (isStart) {
        return;
    }
    const char *path = env->GetStringUTFChars(path_, 0);
    char *url = new char[strlen(path) + 1];
    strcpy(url, path);//放入堆内存中，防止出了方法就被释放了，这个url需要到子线程中使用
    isStart = 1;

    //启动线程 因为需要连接服务器，耗时操作
    pthread_create(&pid, 0, start, url);
    env->ReleaseStringUTFChars(path_, path);
}

extern "C"
JNIEXPORT void JNICALL
Java_lyq_com_lpusher_live_LivePusher_native_1stop(JNIEnv *env, jobject instance) {

    // 停止直播
    readyPushing = 0;
    pthread_join(pid, 0);

}



extern "C"
JNIEXPORT void JNICALL
Java_lyq_com_lpusher_live_LivePusher_native_1pushVideo(JNIEnv *env, jobject instance,
                                                       jbyteArray data_) {

    if (!videoChannel || !readyPushing) {
        return;
    }

    jbyte *data = env->GetByteArrayElements(data_, NULL);
    videoChannel->encodeData(data);
    env->ReleaseByteArrayElements(data_, data, 0);
}


extern "C"
JNIEXPORT void JNICALL
Java_lyq_com_lpusher_live_LivePusher_native_1release(JNIEnv *env, jobject instance) {


    // TODO 释放
    DELETE(videoChannel);
    DELETE(audioChannel) ;

}


extern "C"
JNIEXPORT void JNICALL
Java_lyq_com_lpusher_live_LivePusher_native_1setAudioEncInfo(JNIEnv *env, jobject instance,
                                                             jint sampleRateInHz, jint channels) {

    // TODO 设置录音信息
    if (audioChannel) {
        audioChannel->setAudioEncInfo(sampleRateInHz, channels);
    }

}


extern "C"
JNIEXPORT void JNICALL
Java_lyq_com_lpusher_live_LivePusher_native_1pushAudio(JNIEnv *env, jobject instance,
                                                       jbyteArray data_) {
    //  推音频数据
    if (!audioChannel || !readyPushing) {
        return;
    }

    jbyte *data = env->GetByteArrayElements(data_, NULL);
    audioChannel->encodeData(data);
    env->ReleaseByteArrayElements(data_, data, 0);
}


extern "C"
JNIEXPORT jint JNICALL
Java_lyq_com_lpusher_live_LivePusher_native_1getInputSamples(JNIEnv *env, jobject instance) {

    // TODO
    if (audioChannel) {
        return audioChannel->getInputSamples();
    }

    return -1;
}
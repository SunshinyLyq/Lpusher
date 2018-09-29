package lyq.com.lpusher.live.channel;


import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import lyq.com.lpusher.live.LivePusher;

/**
 * @author liyuqing
 * @date 2018/9/26.
 * @description 写自己的代码，让别人说去吧
 */
public class AudioChannel {

    public static final int SAMPLE_RATE_IN_HZ = 44100;

    private int inputSample;
    private AudioRecord audioRecord;
    private LivePusher mLivePusher;
    private int channels = 2 ;
    private boolean isLiving;
    private ExecutorService executor;

    public AudioChannel(LivePusher livePusher) {
        mLivePusher = livePusher;
        executor= Executors.newSingleThreadExecutor();

        //准备录音机， 采集pcm数据
        int channelConfig;
        if (channels  == 2){
            channelConfig = AudioFormat.CHANNEL_IN_STEREO;
        }else {
            channelConfig = AudioFormat.CHANNEL_IN_MONO;
        }

        mLivePusher.native_setAudioEncInfo(SAMPLE_RATE_IN_HZ,channels);

        //16位 2个字节
        inputSample = mLivePusher.native_getInputSamples() * 2;

        //最小的缓冲区
        int minBufferSize = AudioRecord.getMinBufferSize(SAMPLE_RATE_IN_HZ,channelConfig,AudioFormat.ENCODING_PCM_16BIT) * 2;

        //1.麦克风，2，采样率 3.声道数 4，采样位
        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC,SAMPLE_RATE_IN_HZ,channelConfig,AudioFormat.ENCODING_PCM_16BIT, minBufferSize > inputSample ? minBufferSize:inputSample);

    }

    public void startLive() {
        isLiving = true;
        executor.submit(new AudioTask());
    }

    public void stopLive() {
        isLiving = false;

    }

    public void release() {
        audioRecord.release();
    }

    class AudioTask implements Runnable{

        @Override
        public void run() {
            //启动录音机
            audioRecord.startRecording();
            byte[] bytes=new byte[inputSample];

            while (isLiving){
                int len = audioRecord.read(bytes,0,bytes.length);

                if (len > 0){
                    //送去编码
                    mLivePusher.native_pushAudio(bytes);
                }
            }

            //停止录音
            audioRecord.stop();
        }
    }
}

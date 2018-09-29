package lyq.com.lpusher.live;

import android.app.Activity;
import android.view.SurfaceHolder;
import android.view.View;

import lyq.com.lpusher.live.channel.AudioChannel;
import lyq.com.lpusher.live.channel.VideoChannel;

/**
 * @author liyuqing
 * @date 2018/9/26.
 * @description 写自己的代码，让别人说去吧
 */
public class LivePusher {

    static {
        System.loadLibrary("native-lib");
    }


    private VideoChannel videoChannel;
    private AudioChannel audioChannel;

    public LivePusher(Activity activity,int width,int height,int bitrate,
                      int fps,int cameraId) {

        native_init();
        videoChannel=new VideoChannel(this,activity,width,height,bitrate,fps,cameraId);
        audioChannel=new AudioChannel(this);
    }

    public void setPreviewDisplay(SurfaceHolder surfaceHolder){
        videoChannel.setPreviewDisplay(surfaceHolder);
    }

    /**
     * 切换摄像头
     */
    public void switchCamera() {
        videoChannel.switchCamera();
    }

    /**
     * 开始直播
     */
    public void startLive(String path) {

        native_start(path);
        videoChannel.startLive();
        audioChannel.startLive();
    }

    /**
     * 停止直播
     */
    public void stopLive() {
        videoChannel.stopLive();
        audioChannel.stopLive();
        native_stop();
    }


    public void release(){

        videoChannel.release();
        audioChannel.release();
        native_release();
    }

    public native void native_init();
    public native void native_start(String path);

    public native void native_setVideoEncInfo(int width,int height,int fps,int bitrate);
    public native void native_setAudioEncInfo(int sampleRateInHz,int channels);

    public native void native_pushVideo(byte[] data);
    public native void native_pushAudio(byte[] data);

    public native int native_getInputSamples();

    public native void native_stop();
    public native void native_release();

}

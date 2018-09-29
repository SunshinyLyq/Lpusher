package lyq.com.lpusher.live.channel;

import android.app.Activity;
import android.app.DatePickerDialog;
import android.hardware.Camera;
import android.view.SurfaceHolder;
import android.view.View;

import lyq.com.lpusher.live.LivePusher;

/**
 * @author liyuqing
 * @date 2018/9/26.
 * @description 写自己的代码，让别人说去吧
 */
public class VideoChannel implements Camera.PreviewCallback, CameraHelper.OnChangedSizeListener {

    private LivePusher mLivePusher;
    private CameraHelper cameraHelper;
    private int mBitrate; //码率
    private int mFps; //帧率
    private boolean isLiving;


    public VideoChannel(LivePusher livePusher, Activity activity,int width,int height,int bitrate,int fps,int cameraId) {
        mLivePusher=livePusher;
        mBitrate=bitrate;
        mFps=fps;
        cameraHelper = new CameraHelper(activity,height,width,cameraId);
        //1.设置预览数据的回调
        cameraHelper.setmPreviewCallBack(this);
        //2.回调  真实的摄像头数据宽、高
        cameraHelper.setmOnChangedListener(this);


    }

    public void switchCamera() {
        cameraHelper.switchCamera();
    }

    public void startLive() {
        isLiving=true;
    }

    public void stopLive() {
        isLiving=false;
    }


    public void setPreviewDisplay(SurfaceHolder surfaceHolder) {
        cameraHelper.setPreViewDisplay(surfaceHolder);
    }

    @Override
    public void onPreviewFrame(byte[] bytes, Camera camera) {
        if (isLiving){
            mLivePusher.native_pushVideo(bytes);
        }
    }


    /**
     *
     * 真是摄像头数据的宽、高
     * @param w
     * @param h
     */
    @Override
    public void onChanged(int w, int h) {
        //初始化编码器
        mLivePusher.native_setVideoEncInfo(w,h,mFps,mBitrate);
    }

    public void release() {
        cameraHelper.release();
    }
}

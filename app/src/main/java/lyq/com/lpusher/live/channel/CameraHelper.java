package lyq.com.lpusher.live.channel;

import android.app.Activity;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import java.util.Date;
import java.util.Iterator;
import java.util.List;

/**
 * @author liyuqing
 * @date 2018/9/27.
 * @description 写自己的代码，让别人说去吧
 */
public class CameraHelper implements Camera.PreviewCallback, SurfaceHolder.Callback {

    private static final String TAG = "CameraHelper";

    private Activity mActivity;
    private int mHeight;
    private int mWidth;
    private int mCameraId;
    private Camera mCamera;
    private SurfaceHolder mSurfaceHolder;
    private byte[] buffer;
    private Camera.PreviewCallback mPreviewCallBack;

    private int mRotation;
    private OnChangedSizeListener mOnChangedListener;

    byte[] bytes;

    public CameraHelper(Activity activity, int mHeight, int mWidth, int mCameraId) {
        this.mActivity = activity;
        this.mHeight = mHeight;
        this.mWidth = mWidth;
        this.mCameraId = mCameraId;
    }

    public void setPreViewDisplay(SurfaceHolder surfaceHolder) {
        mSurfaceHolder = surfaceHolder;
        mSurfaceHolder.addCallback(this);
    }

    public void setmPreviewCallBack(Camera.PreviewCallback mPreviewCallBack) {
        this.mPreviewCallBack = mPreviewCallBack;
    }

    //切换摄像头
    public void switchCamera() {
        if (mCameraId == Camera.CameraInfo.CAMERA_FACING_BACK) {
            mCameraId = Camera.CameraInfo.CAMERA_FACING_FRONT;
        } else {
            mCameraId = Camera.CameraInfo.CAMERA_FACING_BACK;
        }

        stopPreview();

        startPreview();
    }

    private void stopPreview() {
        if (mCamera != null) {
            //预览数据回调接口
            mCamera.setPreviewCallback(null);
            //停止预览
            mCamera.stopPreview();
            //释放摄像头
            mCamera.release();
            mCamera = null;
        }
    }

    private void startPreview() {
        try {
            //获得camera对象
            mCamera = Camera.open(mCameraId);
            //配置camera的属性
            Camera.Parameters parameters = mCamera.getParameters();
            //设置预览数据格式为nv21
            parameters.setPreviewFormat(ImageFormat.NV21);
            //这是里摄像头的宽、高
            setPreviewSize(parameters);
            //设置摄像头 图像传感器的角度、方向
            setPreviewOrientation(parameters);
            mCamera.setParameters(parameters);

            //这个数据缓存区大小，跟NV21的数据格式有关，
            //NV21
            buffer = new byte[mWidth * mHeight * 3 / 2];
            bytes=new byte[buffer.length];
            //数据缓存区
            mCamera.addCallbackBuffer(buffer);
            mCamera.setPreviewCallbackWithBuffer(this);

            //设置预览画面
            mCamera.setPreviewDisplay(mSurfaceHolder);
            mCamera.startPreview();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * 因为图像传感器的取景方向 和 手机正常方向是呈90度夹角的，所有需要重新计算一下
     *
     * @param parameters
     */
    private void setPreviewOrientation(Camera.Parameters parameters) {
        Camera.CameraInfo info = new Camera.CameraInfo();
        Camera.getCameraInfo(mCameraId, info);
        //获得当前手机的放置方向
        mRotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();
        int degrees = 0;
        switch (mRotation) {
            case Surface.ROTATION_0:   //观看直播视频逆时针转了90度
                degrees = 0;  // 手机正常方向
                mOnChangedListener.onChanged(mHeight,mWidth);
                break;
            case Surface.ROTATION_90: //横屏 左边是头部（home键在右边）
                degrees = 90;
                mOnChangedListener.onChanged(mWidth,mHeight);
                break;
            case Surface.ROTATION_270: // 横屏，home键在左边
                degrees = 270;
                mOnChangedListener.onChanged(mWidth,mHeight);
                break;

        }

        int result;
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = (info.orientation + degrees) % 360;
            result = (360 - result) % 360;
        } else { //后置摄像头
            result = (info.orientation - degrees + 360) % 360;
        }

        //设置角度
        //设置的是照片的方向
        //预览的方向，如果不旋转，则看到的始终都是逆时针翻转90度的
        //因为摄像头的自然方向和图像方向不一样
        mCamera.setDisplayOrientation(result);

    }

    /**
     * 因为传递过来的宽高，摄像头不一定支持，所以这里需要设置一个差距最小的，且摄像头支持的宽，高
     *
     * @param parameters
     */
    private void setPreviewSize(Camera.Parameters parameters) {
        //获取摄像头支持的宽、高
        List<Camera.Size> supportedPreviewSizes = parameters.getSupportedPreviewSizes();
        Camera.Size size = supportedPreviewSizes.get(0);
        Log.e("自定义标签", "支持  宽：" + size.width + "高：" + size.height);
        //选择一个与设置的差距最小的支持分辨率
        int m = Math.abs(size.height * size.width - mWidth * mHeight);
        supportedPreviewSizes.remove(0);
        Iterator<Camera.Size> iterator = supportedPreviewSizes.iterator();
        //遍历
        while (iterator.hasNext()) {
            Camera.Size next = iterator.next();
            Log.e("自定义标签", "支持  宽：" + next.width + "高：" + next.height);

            int n = Math.abs(next.width * next.height - mWidth * mHeight);
            if (n < m) {
                m = n;
                size = next;
            }
        }

        mWidth = size.width;
        mHeight = size.height;
        parameters.setPreviewSize(mWidth, mHeight);

        Log.e("自定义标签", "设置预览分辨率:  width" + size.width + "height: " + size.height);

    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {

        switch (mRotation){
            case Surface.ROTATION_0:
                rotation90(data);
                break;
            case Surface.ROTATION_90:
                break;
            case Surface.ROTATION_270:
                break;
        }

        //回调采集到的数据，这里的数据格式为NV21,而x264需要的数据格式为I420
        mPreviewCallBack.onPreviewFrame(bytes, camera);
        //数据依旧是倒着的
        camera.addCallbackBuffer(buffer);
    }

    private void rotation90(byte[] data) {
        int index = 0 ;
        int ySize = mWidth * mHeight;
        int uvHeight = mHeight / 2 ;

        //后置摄像头顺时针旋转90度
        if (mCameraId == Camera.CameraInfo.CAMERA_FACING_BACK){
            //将y数据顺时针旋转90度
            for (int i = 0; i < mWidth; i++) {
                for (int j = mHeight - 1; j >=0; j--) {
                    bytes[index++] = data[mWidth * j + i];
                }
            }

            //uv数据旋转90度
            for (int i = 0; i < mWidth; i+=2) {
                for (int j = uvHeight - 1; j >= 0; j--) {
                    bytes[index++] = data[ySize + mWidth * j + i];
                    bytes[index++] = data[ySize + mWidth * j + i +1];
                }

            }

        }else{
            //前置摄像头，逆时针旋转90度

            //y
            for (int i = 0; i < mWidth; i++) {
                int nPos = mWidth - 1;
                for (int j = 0; j < mHeight; j++) {
                    bytes[index++] =data[nPos - i];
                    nPos += mWidth;
                }
            }

            //uv
            for (int i = 0; i < mWidth; i+=2) {
                int nPos = ySize + mWidth -1;
                for (int j = 0; j < uvHeight; j++) {
                    bytes[index++] = data[nPos -i -1];
                    bytes[index++] = data[nPos - i];
                    nPos += mWidth;
                }
            }

        }

    }

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        //释放摄像头
        stopPreview();
        //开启
        startPreview();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
        stopPreview();
    }

    public void setmOnChangedListener(OnChangedSizeListener mOnChangedListener) {
        this.mOnChangedListener = mOnChangedListener;
    }

    public void release() {
        mSurfaceHolder.removeCallback(this);
        stopPreview();
    }

    public interface OnChangedSizeListener {
        void onChanged(int w, int h);
    }
}

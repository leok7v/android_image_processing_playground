package ip.c0d.ippg;

import android.graphics.*;
import android.hardware.Camera;

import java.io.*;
import java.util.*;

import static jni.libyuv.*;
import static jni.Native.*;
import static misc.util.*;

public class SurfaceCamera {

    private Camera camera;
    private int width;
    private int height;
    private int bpp;
    private long abgr;
    private long gray;
    private SurfaceTexture st; // != null only while streaming see stop() and close()
    private static final int TEXTURE_ID = 10; // magic constant via OpenCV
    private FrameListener listener;

    public interface FrameListener {
        public void frame(long buffer, int width, int height, int bytesPerPixel);
    }

    public int width() { return width; }

    public int height() { return height; }

    public int bpp() { return bpp; }

    public void setCallback(FrameListener capture) { listener = capture; }

    public boolean open(int id, int w, int h, int bytesPerPixel) {
        assertion(camera == null, "camera already initialized");
        width = w;
        height = h;
        bpp = bytesPerPixel;
        abgr = malloc(width * height * 4);
        gray = malloc(width * height);
        listener = null;
        Camera.CameraInfo info = new Camera.CameraInfo();
        // Try to find a front-facing camera (e.g. for videoconferencing).
        int n = Camera.getNumberOfCameras();
        for (int i = 0; i < n; i++) {
            Camera.getCameraInfo(i, info);
            if (info.facing == id) {
                try {
                    camera = Camera.open(i);
                    break;
                } catch (Throwable t) {
                    trace("camera open failure: " + t.getMessage());
                }
            }
        }
        if (camera == null) {
            return false;
        }
        Camera.Parameters ps = camera.getParameters();
        choosePreviewSize(ps, width, height);
        camera.setParameters(ps);
        setParameters();
        return true;
    }

    public void close() {
        if (camera != null && st != null) {
            stop();
        }
        if (camera != null) {
            camera.release();
            camera = null;
            st = null;
        }
    }

    public boolean start() {
        if (camera == null) {
            return false; // this call has been posted on the queue... camera can be closed by now
        }
        // TODO: simplify according to:
        // http://stackoverflow.com/questions/10775942/android-sdk-get-raw-preview-camera-image-without-displaying-it
        // and openCV source code does it even simpler
        // "You can simply create a dummy SurfaceTexture object without any OpenGL context set up. Pass any integer
        //  you want as the texture ID, and connect the SurfaceTexture to the camera using setPreviewTexture.
        //  As long as you don't call updateTexImage, the SurfaceTexture will simply discard all data passed
        // into it by the camera."
        st = new SurfaceTexture(TEXTURE_ID);
        st.setDefaultBufferSize(width, height);
        setPreviewTexture(st);
        camera.setPreviewCallback(new Camera.PreviewCallback() {
            public void onPreviewFrame(byte[] data, Camera camera) {
                if (bpp == 4) {
                    NV21toABGR(data, abgr, width, height); // 8 - 15 ms
                } else {
                    memcpy(gray, data, 0, width * height); // only Y component of YUV
                }
                if (listener != null) {
                    listener.frame(bpp == 4 ? abgr : gray, width, height, bpp);
                }
            }
        });
        camera.startPreview();
        return true;
    }

    public boolean stop() {
        if (camera != null && st != null) {
            camera.setPreviewCallback(null);
            camera.stopPreview();
            st.detachFromGLContext();
            st = null;
        }
        return camera != null;
    }

    private void setPreviewTexture(SurfaceTexture s) {
        try {
            camera.setPreviewTexture(s);
        } catch (IOException e) {
            throw new Error(e);
        }
    }

    private boolean setParams(String m, Camera.Parameters p) {
        try {
            camera.setParameters(p);
            return true;
        } catch (Throwable t) {
            trace(m + t.getMessage()); /* can be ignored for now */
            return false;
        }
    }

    @SuppressWarnings("deprecation")
    private void setParameters() {
        Camera.Parameters p = camera.getParameters();
        if (p.isAutoWhiteBalanceLockSupported()) {
            p.setAutoWhiteBalanceLock(true);
        }
        if (p.isAutoExposureLockSupported()) {
            p.setAutoExposureLock(true);
        }
        p.setWhiteBalance(Camera.Parameters.WHITE_BALANCE_DAYLIGHT);
        setParams("lock exposure & white balance ", p);

        p.setWhiteBalance(Camera.Parameters.WHITE_BALANCE_DAYLIGHT);

        List<int[]> pr = p.getSupportedPreviewFpsRange();
        if (pr != null && pr.size() > 0) {
            int[] max = pr.get(0);
            for (int[] e : pr) {
                if (e[1] > max[1]) {
                    max = e;
                }
            }
            int[] range = new int[2];
            p.getPreviewFpsRange(range);
            p.setPreviewFpsRange(C.FPS * 1000, 60 * 1000);
            if (!setParams("preview FPS range ", p)) {
                Camera.Parameters fps = camera.getParameters();
                fps.setPreviewFrameRate(C.FPS); // deprecated but still works
                setParams("setPreviewFrameRate ", fps);
            }
        }
    }

    private static void choosePreviewSize(Camera.Parameters params, int w, int h) {
        for (Camera.Size s : params.getSupportedPreviewSizes()) {
            if (s.width == w && s.height == h) {
                params.setPreviewSize(w, h);
                return;
            }
        }
        throw new Error("Unable to set preview size to " + w + "x" + h);
    }

}

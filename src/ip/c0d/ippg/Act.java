package ip.c0d.ippg;

import android.hardware.*;
import android.os.Bundle;
import android.view.*;

import static misc.util.*;

public class Act extends BaseActivity {

    private SurfaceCamera camera = new SurfaceCamera();
    private MainView mainView;
    private FrameListener frameListener = new FrameListener();

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        mainView = new MainView(this);
        setContentView(mainView);
        camera.open(Camera.CameraInfo.CAMERA_FACING_FRONT, C.WIDTH, C.HEIGHT, C.BPP);
    }

    public void onResume() {
        camera.start();
        camera.setCallback(frameListener);
        super.onResume();
    }

    public void onPause() {
        camera.setCallback(null);
        camera.stop();
        super.onPause();
    }

    public void onDestroy() {
        super.onDestroy();
        camera.close();
        camera = null;
    }

    private class FrameListener implements SurfaceCamera.FrameListener {

        public void frame(long buffer, int w, int h, int bpp) {
            assertion(isMainThread(), "this callback was expected to be executed on main thread");
            mainView.onFrameReady(buffer, w, h, bpp);
        }

    }

}

package ip.playground;

import android.content.*;
import android.view.*;
import jni.*;
import misc.*;

import static android.view.View.MeasureSpec.*;
import static jni.Native.*;

public class MainView extends ViewGroup implements TopView {

    private boolean attached;
    private BitmapView[] bitmapViews = new BitmapView[4];
    private long outputThreshold;
    private long outputDilate;
    private static final int PADDING = 8, GAP_BETWEEN = 16;

    public MainView(final Context ctx) {
        super(ctx);
        setWillNotDraw(false);
        setWillNotCacheDrawing(true);
        setBackgroundColor(C.BACKGROUND_COLOR);
        createViews();
        outputThreshold = malloc(C.WIDTH * C.HEIGHT);
        outputDilate = malloc(C.WIDTH * C.HEIGHT);
    }

    public void onFrameReady(long buffer, int w, int h, int bpp) {
        if (attached) {
            bitmapViews[0].update(buffer, w, h, bpp);
            ImageProcessing.threshold8u(buffer, w, w, h, 200, 255, outputThreshold);
            bitmapViews[1].update(outputThreshold, w, h, bpp);
            ImageProcessing.dilate8unz(outputThreshold, w, w, h, 1, 255, outputDilate);
            bitmapViews[2].update(outputDilate, w, h, bpp);
        }
    }

    public void onStart() { }

    public void onResume() { }

    public void onPause() { }

    public void onRestart() { }

    public void onStop() { }

    public void onDestroy() {
        if (bitmapViews != null) {
            removeAllViews();
            free(outputThreshold);
            free(outputDilate);
            outputThreshold = outputDilate = 0;
            bitmapViews = null;
        }
    }

    private void createViews() {
        Context c = getContext();
        for (int i = 0; i < bitmapViews.length; i++) {
            bitmapViews[i] = new BitmapView(c).setSize(C.WIDTH, C.HEIGHT);
            addView(bitmapViews[i]);
        }
    }

    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        attached = true;
    }

    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        attached = false;
    }

    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        int k = 0;
        int w = Math.min(C.WIDTH, (getMeasuredWidth() - GAP_BETWEEN - PADDING * 2) / 2);
        int h = Math.min(C.HEIGHT, (getMeasuredHeight() - GAP_BETWEEN - PADDING * 2) / 2);
        for (int i = 0; i < getChildCount(); i++) {
            View c = getChildAt(i);
            if (c instanceof BitmapView) {
                int x = (k % 2) * w + (k % 2 > 0 ? GAP_BETWEEN : 0) + PADDING;
                int y = (k / 2) * h + (k / 2 > 0 ? GAP_BETWEEN : 0) + PADDING;
                c.layout(x, y, x + w, y + h);
                k++;
            }
        }
    }

    protected void onMeasure(int wms, int hms) {
        super.onMeasure(wms, hms);
        int w = Math.min(C.WIDTH, (getMeasuredWidth() - GAP_BETWEEN - PADDING * 2) / 2);
        int h = Math.min(C.HEIGHT, (getMeasuredHeight() - GAP_BETWEEN - PADDING * 2) / 2);
        if (w > 0 && h > 0) {
            int n = getChildCount();
            for (int i = 0; i < n; i++) {
                View c = getChildAt(i);
                if (c instanceof BitmapView) {
                    c.measure(makeMeasureSpec(w, EXACTLY), makeMeasureSpec(h, EXACTLY));
                }
            }
        }
        setMeasuredDimension(util.measure(getMode(wms), getSize(wms), w), util.measure(getMode(hms), getSize(hms), h));
    }

}



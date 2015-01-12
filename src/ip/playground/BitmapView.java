package ip.playground;

import android.content.*;
import android.graphics.*;
import android.view.*;
import com.ortiz.touch.*;
import jni.*;
import static misc.util.*;

public class BitmapView extends TouchImageView {

    private Bitmap bitmap;
    private int width;
    private int height;

    public BitmapView(Context context) {
        super(context);
    }

    public BitmapView setSize(int w, int h) {
        if (w != width || h != height) {
            width = w;
            height = h;
            getBitmap(width, height);
            requestLayout();
        }
        return this;
    }

    public void layout(int l, int t, int r, int b) {
        super.layout(l, t, r, b);
        setMinZoom(1f);
        setMaxZoom(4.0f);
        setZoom(1.0f, 0.5f, 0.5f, ScaleType.FIT_CENTER);
    }

    public void update(long address, int w, int h, int bpp) {
        assertion(isMainThread());
        if (getParent() != null) {
            Android.updateBitmap(getBitmap(width, height), address, w, h, bpp, true);
            invalidate();
        }
    }

    private Bitmap getBitmap(int w, int h) {
        if (bitmap != null && (bitmap.getWidth() != w || bitmap.getHeight() != h)) {
            bitmap.recycle();
            bitmap = null;
        }
        if (bitmap == null) {
            bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        }
        setImageBitmap(bitmap);
        return bitmap;
    }

    protected void onDraw(Canvas c) {
        c.drawColor(C.BITMAP_VIEW_BACKGROUND_COLOR);
        super.onDraw(c);
    }

}



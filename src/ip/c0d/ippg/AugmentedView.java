package ip.c0d.ippg;

import android.content.*;
import android.graphics.*;
import jni.*;

public class AugmentedView extends BitmapView {

    private int nb;
    private float[] fp = new float[2];
    private ImageProcessing.Blob[] blobs = new ImageProcessing.Blob[1];
    private double thresholdTime;
    private double dilateTime;
    private double findBlobsTime;

    private static final Paint red = new Paint() {{
        setAntiAlias(true);
        setColor(Color.RED);
        setStyle(Paint.Style.STROKE);
        setStrokeJoin(Paint.Join.ROUND);
        setStrokeWidth(3f);
        setTypeface(Typeface.MONOSPACE);
        setTextSize(20f);
    }};
    private static final Paint green = new Paint(red) {{ setColor(Color.GREEN); }};
    private static final Paint blue = new Paint(red) {{ setColor(Color.BLUE); }};
    private static final Paint yellow = new Paint(red) {{ setColor(Color.YELLOW); }};
//  private static final Paint magenta = new Paint(red) {{ setColor(Color.MAGENTA); }};
    private static final Matrix matrix = new Matrix() {{ postScale(-1, 1); postTranslate(640, 0); }};
    private static final Paint text = new Paint(yellow) {{
        setStyle(Paint.Style.STROKE);
        setStrokeJoin(Paint.Join.ROUND);
        setStrokeWidth(1f);
    }};

    public AugmentedView(Context context) {
        super(context);
    }

    public void update(int nb, ImageProcessing.Blob[] bs, int np, int[] points,
            double thresholdTime, double dilateTime, double findBlobsTime) {
        this.nb = nb;
        if (this.blobs.length < nb) {
            this.blobs = new ImageProcessing.Blob[nb];
        }
        if (fp.length < np * 2) {
            fp = new float[np * 2];
        }
        System.arraycopy(bs, 0, this.blobs, 0, nb);
        for (int i = 0; i < np * 2; i++) {
            fp[i] = points[i];
        }
        this.thresholdTime = thresholdTime;
        this.dilateTime = dilateTime;
        this.findBlobsTime = findBlobsTime;
        invalidate();
    }

    protected void onDraw(Canvas c) {
        c.save(Canvas.MATRIX_SAVE_FLAG);
        super.onDraw(c);
        c.concat(matrix);
        for (int i = 0; i < nb; i++) {
            ImageProcessing.Blob b = blobs[i];
//          trace("blobs[" + i + "] center " + b.centerX + "," + b.centerY);
//          trace("blobs[" + i + "] numberOfSegments " + b.numberOfSegments + " segmentsStars " + b.segmentsStart);
            if (i < 100) {
                c.drawLines(fp, b.segmentsStart * 2, b.numberOfSegments * 2, red);
            }
            c.drawRect(b.left, b.top, b.right, b.bottom, green);
            c.drawRect(b.centerX, b.centerY, b.centerX + 1, b.centerY + 1, blue);
        }
        c.restore();
        c.drawText(String.format("threshold  %.6f", thresholdTime), 10, 20, text);
        c.drawText(String.format("dilate     %.6f", dilateTime), 10, 40, text);
        c.drawText(String.format("find blobs %.6f", findBlobsTime), 10, 60, text);
        c.drawText(String.format("total      %.6f", thresholdTime + dilateTime + findBlobsTime), 10, 80, text);
    }

}



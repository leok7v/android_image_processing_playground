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
    private static final Matrix matrix = new Matrix() {{ postScale(-1, 1); postTranslate(640, 0); }};
    private static final float[] matrix_elements = new float[9];

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
    private static final Paint coral1 = new Paint(red) {{ setColor(Colors.Coral1); }};
    private static final Paint text = new Paint(green) {{
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
        int saveCount = c.getSaveCount();
        c.save(Canvas.MATRIX_SAVE_FLAG);
        super.onDraw(c);
        getImageMatrix().getValues(matrix_elements);
        matrix_elements[Matrix.MTRANS_X] += getMeasuredWidth() * matrix_elements[Matrix.MSCALE_X];
        matrix_elements[Matrix.MSCALE_X] = -matrix_elements[Matrix.MSCALE_X];
        matrix.setValues(matrix_elements);
        c.concat(matrix);
        getImageMatrix().getValues(matrix_elements);
        for (int i = 0; i < nb; i++) {
            ImageProcessing.Blob b = blobs[i];
//          trace("blobs[" + i + "] center " + b.centerX + "," + b.centerY);
//          trace("blobs[" + i + "] numberOfSegments " + b.numberOfSegments + " segmentsStars " + b.segmentsStart);
            c.drawLines(fp, b.segmentsStart * 2, b.numberOfSegments * 2, coral1);
            c.drawRect(b.left, b.top, b.right, b.bottom, blue);
            c.drawRect(b.centerX, b.centerY, b.centerX + 1, b.centerY + 1, red);
        }
        c.restoreToCount(saveCount);
        c.drawText(String.format("threshold  %.6f", thresholdTime), 10, 20, text);
        c.drawText(String.format("dilate     %.6f", dilateTime), 10, 40, text);
        c.drawText(String.format("find blobs %.6f", findBlobsTime), 10, 60, text);
        c.drawText(String.format("total      %.6f", thresholdTime + dilateTime + findBlobsTime), 10, 80, text);
    }

}



package ip.c0d.ippg;

import android.content.*;
import android.graphics.*;
import jni.*;

public class AugmentedView extends BitmapView {

    private int nb;
    private float[] fp = new float[2];
    private ImageProcessing.Blob[] blobs = new ImageProcessing.Blob[1];
    private static final Matrix matrix = new Matrix() {{ postScale(-1, 1); postTranslate(640, 0); }};
    private static final float[] matrix_elements = new float[9];

    private static final Paint red = new Paint() {{
        setAntiAlias(true);
        setColor(Color.RED);
        setStyle(Paint.Style.STROKE);
        setStrokeJoin(Paint.Join.ROUND);
        setStrokeWidth(2f);
    }};
    private static final Paint blue = new Paint(red) {{ setColor(Color.BLUE); }};
    private static final Paint coral1 = new Paint(red) {{ setColor(Colors.Coral1); }};

    public AugmentedView(Context context) {
        super(context);
    }

    public void update(int nb, ImageProcessing.Blob[] bs, int np, int[] points) {
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
            if (b.numberOfSegments > 1) {
                c.drawLines(fp, b.segmentsStart * 2, b.numberOfSegments * 2, coral1);
                c.drawRect(b.left, b.top, b.right, b.bottom, blue);
            }
            c.drawRect(b.centerX, b.centerY, b.centerX + 1, b.centerY + 1, red);
        }
        c.restoreToCount(saveCount);
    }

}

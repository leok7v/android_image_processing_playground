package ip.c0d.ippg;

import android.content.*;
import android.graphics.*;
import jni.*;

import static misc.util.assertion;

public class AugmentedView extends BitmapView {

    private int nb;
    private ImageProcessing.Blob[] blobs = new ImageProcessing.Blob[1];
    private Path[] path = new Path[blobs.length];
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
            Path[] ps = new Path[nb];
            System.arraycopy(path, 0, ps, 0, path.length);
            path = ps;
        }
        System.arraycopy(bs, 0, this.blobs, 0, nb);
        for (int i = 0; i < nb; i++) {
            Path p = path[i];
            if (p == null) {
                p = new Path();
                path[i] = p;
            }
            p.reset();
            ImageProcessing.Blob b = blobs[i];
            if (b.numberOfSegments > 1) {
                int ix = b.segmentsStart * 2;
                p.moveTo(points[ix++], points[ix++]);
                for (int j = 1; j < b.numberOfSegments; j++) {
                    assertion(ix < np * 2);
                    int x = points[ix++];
                    int y = points[ix++];
                    assertion(0 <= x && x < width);
                    assertion(0 <= y && y < height);
                    p.lineTo(x, y);
                }
                p.close();
            }
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
                c.drawPath(path[i], coral1);
                c.drawRect(b.left, b.top, b.right, b.bottom, blue);
            }
            c.drawRect(b.centerX, b.centerY, b.centerX + 1, b.centerY + 1, red);
        }
        c.restoreToCount(saveCount);
    }

}

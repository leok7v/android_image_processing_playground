package ip.c0d.ippg;

import android.content.*;
import android.graphics.*;
import android.view.*;
import jni.*;
import misc.*;

public class StatsView extends View {

    private static final int SECONDS = 10;
    MovingAverage thresholdAverage = new MovingAverage(C.FPS * SECONDS);
    MovingAverage dilateAverage = new MovingAverage(C.FPS * SECONDS);
    MovingAverage findBlobsAverage = new MovingAverage(C.FPS * SECONDS);
    MovingAverage totalAverage = new MovingAverage(C.FPS * SECONDS);

    MovingAverage nbAverage = new MovingAverage(C.FPS * SECONDS);
    MovingAverage npAverage = new MovingAverage(C.FPS * SECONDS);
    MovingAverage npPerBlobAverage = new MovingAverage(C.FPS * SECONDS);

    private static final Paint text = new Paint() {{
        setAntiAlias(true);
        setStyle(Style.STROKE);
        setStrokeJoin(Join.ROUND);
        setStrokeWidth(1f);
        setTypeface(Typeface.MONOSPACE);
        setTextSize(20f);
        setColor(Color.GREEN);
    }};

    public StatsView(Context context) {
        super(context);
        setEnabled(false);
    }

    public void update(int nb, ImageProcessing.Blob[] bs, int np,
                       double thresholdTime, double dilateTime, double findBlobsTime) {
        thresholdAverage.push(thresholdTime);
        dilateAverage.push(dilateTime);
        findBlobsAverage.push(findBlobsTime);
        totalAverage.push(thresholdTime + dilateTime + findBlobsTime);
        int max_np_perBlob = 0;
        for (int i = 0; i < nb; i++) {
            if (bs[i].numberOfSegments > max_np_perBlob) {
                max_np_perBlob = bs[i].numberOfSegments;
            }
        }
        npAverage.push(np);
        nbAverage.push(nb);
        npPerBlobAverage.push(max_np_perBlob);
        invalidate();
    }


    private String formatF(String label, MovingAverage ma) {
        return String.format("%s %6.3f [%5.3f .. %5.3f] %6.3f %6.3f",
                label, ma.last * 1000, ma.min * 1000, ma.max * 1000, ma.simple * 1000, ma.cumulative * 1000);
    }

    private String formatI(String label, MovingAverage ma) {
        return String.format("%s  %6.0f [%5.0f .. %5.0f]  %5.0f  %5.0f",
                label, ma.last, ma.min, ma.max, ma.simple, ma.cumulative);
    }

    protected void onDraw(Canvas c) {
        thresholdAverage.minmax();
        dilateAverage.minmax();
        findBlobsAverage.minmax();
        totalAverage.minmax();
        npAverage.minmax();
        nbAverage.minmax();
        npPerBlobAverage.minmax();
        c.drawText("(millisecs) last [min   ..   max]    avg    cma", 10,  20, text);
        c.drawText(formatF("threshold", thresholdAverage), 10,  40, text);
        c.drawText(formatF("dilate   ", dilateAverage),    10,  60, text);
        c.drawText(formatF("find blob", findBlobsAverage), 10,  80, text);
        c.drawText(formatF("total    ", totalAverage),     10, 100, text);

        c.drawText(formatI("#blobs  ", nbAverage), 10, 130, text);
        c.drawText(formatI("#points ", npAverage), 10, 150, text);
        c.drawText(formatI("#pt/blob", npPerBlobAverage), 10, 170, text);
    }

}

/*
in a random noise experiments see so far:
max number of points=2874 blobs=123 max points per blob=697
max times seen threshold=0.001311 threshold=0.003353 threshold=0.002544 total=0.006197 of a second
*/



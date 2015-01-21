package jni;

public class ImageProcessing {

    public static class Blob {
        public int numberOfSegments;
        public int segmentsStart; /* ip_context.segments[segments_start .. segments_start + number_of_segments - 1] */
        public float area; /* area estimate of the blob */
        public float arcLength; /* perimeter */
        public float roundness; /* 1.0 round, 0 not even close */
        public float centerX;   /* mass center of blob */
        public float centerY;
        /* bounding_rect */
        public int left;
        public int top;
        public int right;
        public int bottom;
    }

    private long context;

    public ImageProcessing(int stride, int w, int h) {
        context = create(stride, w, h);
        if (context == 0) {
            throw new OutOfMemoryError();
        }
    }

    public void destroy() {
        if (context != 0) {
            destroy(context);
            context = 0;
        }
    }

    public void threshold(long input, int threshold, int setTo, long output) {
        threshold(context, input, threshold, setTo, output);
    }

    public void dilate(long input, int radius, int setTo, long output) {
        dilate(context, input, radius, setTo, output);
    }

    public boolean findBlobs(long input) { return findBlobs(context, input); }

    public int numberOfBlobs() { return numberOfBlobs(context); }

    public void getBlob(int i, Blob b) { getBlob(context, i, b); }


    public int numberOfPoints() { return numberOfPoints(context); }

    public long segments() { return segments(context); }

    public double thresholdTime() { return thresholdTime(context); }

    public double dilateTime() { return dilateTime(context); }

    public double findBlobsTime() { return findBlobsTime(context); }

    private static native long create(int stride, int w, int h);

    /* "threshold" replaces all the unsigned bytes in the "input" image >= "threshold"
       in the rectangular region of width "w" and height "h" by the value of "setTo" (usually 1 or 255)
       and writes to the "output" image.
       "output" and "input" may point to the exactly same memory location
       and if not they should not overlap.
       "stride" is size in bytes of each row of input and output images.
     */
    private static native void threshold(long context, long input, int threshold, int setTo, long output);

    /* "dilate" replaces all east-west and north-south neighbours in the vicinity of a "radius"
        of none zero bytes in the "input" image in the rectangular region of width "w" and height "h"
        by the value of "setTo"  (usually 1 or 255) and writes to the "output" image.
        "output" and "input" may point to the exactly same memory location
        and if not they should not overlap.
        "stride" is size in bytes of each row of input and output images.
     */
    private static native void dilate(long context, long input, int radius, int setTo, long output);

    private static native boolean findBlobs(long context, long input);

    private static native int numberOfBlobs(long context);

    private static native void getBlob(long context, int i, Blob b);

    private static native int numberOfPoints(long context);

    private static native long segments(long context);

    private static native double thresholdTime(long context);

    private static native double dilateTime(long context);

    private static native double findBlobsTime(long context);

    private static native void destroy(long handle);
}

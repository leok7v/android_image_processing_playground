package jni;

public class ImageProcessing {

    /* "threshold8u" replaces all the unsigned bytes in the "input" image >= "value"
       in the rectangular region of width "w" and height "h" by the value of "set"
       and writes to the "output" image.
       "output" and "input" may point to the exactly same memory location
       and if not they should not overlap.
       "stride" is size in bytes of each row of input and output images.
     */
    public static native void threshold8u(long input, int stride, int w, int h, int value, int set, long output);

    /* "dilate8unz" replaces all east-west and north-south neighbours in the vicinity of a "radius"
        of none zero bytes in the "input" image in the rectangular region of width "w" and height "h"
        by the value of "set" and writes to the "output" image.
        "output" and "input" may point to the exactly same memory location
        and if not they should not overlap.
        "stride" is size in bytes of each row of input and output images.
     */
    public static native void dilate8unz(long input, int stride, int w, int h, int radius, int set, long output);

    private ImageProcessing() { }
}

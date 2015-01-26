package jni;

@SuppressWarnings({"unused"})
public class libyuv {

    public static native void NV21toABGR(byte[] data, long abgr, int w, int h);
    public static native void YV12toABGR(byte[] data, long abgr, int w, int h);

    private libyuv() { }
}

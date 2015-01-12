package jni;

public class libyuv {

    public static native void NV21toABGR(byte[] data, long abgr, int w, int h);

    private libyuv() { }
}

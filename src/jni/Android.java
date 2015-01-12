package jni;

@SuppressWarnings({"unused"})
public class Android {

    public static native void updateBitmap(android.graphics.Bitmap bitmap, long data, int w, int h, int bpp, boolean mirror);

    public static native void setThreadAffinityNextCore();

    public native static void clearMemoryLeaks();
    public native static void checkMemoryLeaks();
    public native static void traceAllocs(boolean b);

    private Android() { }

}

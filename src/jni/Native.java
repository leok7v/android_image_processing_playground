package jni;

@SuppressWarnings({"unused"})
public class Native {

    public static native void memcpy(long to, long from, int bytes);
    public static native long malloc(int bytes);
    public static native void free(long address);

    public static void memcpy(long address, byte[] data) {
        memcpy(address, data, 0, data.length);
    }

    public static void memcpy(byte[] data, long address) {
        memcpy(data, address, 0, data.length);
    }

    public static void memcpy(int[] data, long address) {
        memcpy(data, address, 0, data.length);
    }

    public static void memcpy(long address, byte[] data, int offset, int length) {
        memcpyAB(address, data, offset, length);
    }

    public static void memcpy(byte[] data, long address, int offset, int length){
        memcpyBA(data, address, offset, length);
    }

    public static void memcpy(long address, int[] data, int offset, int length) {
        memcpyAI(address, data, offset, length);
    }

    public static void memcpy(int[] data, long address, int offset, int length){
        memcpyIA(data, address, offset, length);
    }

    public static void memcpy(long address, float[] data, int offset, int length) {
        memcpyAF(address, data, offset, length);
    }

    public static void memcpy(float[] data, long address, int offset, int length){
        memcpyFA(data, address, offset, length);
    }

    public static native int strlen(long address);

    private static native void memcpyAB(long address, byte[] data, int offset, int length);
    private static native void memcpyBA(byte[] data, long address, int offset, int length);
    private static native void memcpyAI(long address, int[] data, int offset, int length);
    private static native void memcpyIA(int[] data, long address, int offset, int length);
    private static native void memcpyAF(long address, float[] data, int offset, int length);
    private static native void memcpyFA(float[] data, long address, int offset, int length);

    private Native() { }

}

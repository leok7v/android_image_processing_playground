package misc;

import android.content.*;
import android.content.res.*;
import android.os.*;
import android.util.*;
import android.view.*;
import jni.*;

import java.io.*;
import java.util.*;

import static android.view.View.MeasureSpec.*;

@SuppressWarnings({"unused"})
public final class util {

    private static final Looper mainLooper = Looper.getMainLooper();
    private static final Handler mainHandler = new Handler(mainLooper);
    private static Thread mainThread = mainLooper.getThread();

    private util() {}

    public static boolean equals(Object o1, Object o2) {
        return o1 == null ? o2 == null : o1.equals(o2);
    }

    public static boolean equals(CharSequence ca1, CharSequence ca2) {
        if (ca1 == ca2) {
            return true;
        } else if (ca1 != null && ca2 != null && ca1.length() == ca2.length()) {
            int n = ca1.length();
            for (int i = 0; i < n; i++) {
                if (ca1.charAt(i) != ca2.charAt(i)) {
                    return false;
                }
            }
            return true;
        } else {
            return false;
        }
    }

    public static float unitToPixels(int unit, float size) {
        DisplayMetrics dm = Resources.getSystem().getDisplayMetrics();
        if (dm.xdpi == 75 && dm.widthPixels >= 1920 && dm.heightPixels >= 1000) {
            dm.xdpi = 100; // most probably unrecognized HDMI monitor with dpi > 90
            dm.ydpi = 100;
        }
        return TypedValue.applyDimension(unit, size, dm);
    }

    public static void invalidateAll(View v) {
        if (v != null) {
            v.invalidate();
            if (v instanceof ViewGroup) {
                ViewGroup vg = (ViewGroup)v;
                int n = vg.getChildCount();
                for (int i = 0; i < n; i++) {
                    invalidateAll(vg.getChildAt(i));
                }
            }
        }
    }

    public static int numberOfBits(int bitset) { // http://en.wikipedia.org/wiki/Hamming_weight
         bitset = bitset - ((bitset >>> 1) & 0x55555555);
         bitset = (bitset & 0x33333333) + ((bitset >>> 2) & 0x33333333);
         return (((bitset + (bitset >>> 4)) & 0x0F0F0F0F) * 0x01010101) >>> 24;
    }

    public static String i2b(int bitset) {
        StringBuilder sb = new StringBuilder(34);
        for (int i = 0; i < 32; i++) {
            sb.append((bitset & 0x1) != 0 ? '1' : '0');
            bitset >>>= 1;
        }
        return "0b" + sb;
    }

    public static boolean isMainThread() {
        return mainThread == Thread.currentThread();
    }

    public static void assertion(boolean b) {
        if (!b) {
            trace("assertion failed");
            rethrow(new AssertionError());
        }
    }

    public static void assertion(boolean b, String msg) {
        if (!b) {
            trace("assertion " + msg + " failed");
            rethrow(new AssertionError(msg));
        }
    }

    public static void rethrow(Throwable t) {
        if (t != null) {
            throw t instanceof Error ? (Error)t : new Error(t);
        }
    }

    public static void trace(String... params) {
        if (params != null && params.length > 0) {
            trace0(getCallersPackage(4), getCallersCaller(4), params);
        }
    }

    public static void trace(Throwable t) {
        trace(t, new String[]{null});
    }

    public static String getCallersCaller(int n) {
        StackTraceElement st = Thread.currentThread().getStackTrace()[n];
        return forName(st.getClassName()).getSimpleName() + "." + st.getMethodName() +
                                ":" + st.getLineNumber() + " ";
    }

    public static String getCallersPackage(int n) {
        StackTraceElement st = Thread.currentThread().getStackTrace()[n];
        return forName(st.getClassName()).getPackage().getName();
    }

    public static Class<?> forName(String n) {
        try {
            return n == null ? null : Class.forName(n);
        } catch (ClassNotFoundException e) {
            return null;
        }
    }

    public static void trace(Throwable t, String... params) {
        ByteArrayOutputStream ba = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(ba);
        if (params != null && params.length > 0) {
            for (String p : params) {
                if (p != null) {
                    ps.print(p);
                    ps.print(' ');
                }
            }
            ps.println();
        }
        t.printStackTrace(ps);
        ps.close();
        trace(ba.toString());
    }

    public static void trace0(String pkg, String caller, String... params) {
        if (params != null && params.length > 0) {
            StringBuilder sb = new StringBuilder(params[0].length() * 2);
            sb.append(caller).append(' ');
            for (String p : params) {
                if (p != null) {
                    sb.append(p).append(' ');
                }
            }
            String s = sb.toString().trim();
            Log.d(pkg, s);
//          System.err.println(s);
        }
    }

    private static HashMap<String, Long> start = new HashMap<String, Long>();
    private static long self_time; // = 0
    private static boolean trace_timestamp;

    public synchronized static long timestamp(String label) { // returns delta in nanoseconds
        if (self_time == 0) {
            self_time = 1;
            timestamp("timestamp-self-delta");
            self_time = timestamp("timestamp-self-delta");
            if (self_time <= 0) {
                self_time = 1;
            }
            trace_timestamp = true;
        }
        long t = System.nanoTime();
        Long s = start.remove(label);
        if (s == null) {
            start.put(label, t);
            return 0;
        } else {
            long delta = t - s;
            delta = delta < 1 ? 1 : delta;
            if (trace_timestamp) {
                trace0(getCallersPackage(4), getCallersCaller(4), "time: \"" + label + "\" " + humanReadable(delta));
            }
            return delta;
        }
    }

    public static String humanReadable(long delta) {
        if (delta < 10L * 1000) {
            return delta + " nanoseconds";
        } else if (delta < 10L * 1000 * 1000) {
            return delta / 1000 + " microseconds";
        } else if (delta < 10L * 1000 * 1000 * 1000) {
            return delta / (1000 * 1000) + " milliseconds";
        } else {
            return delta / (1000 * 1000 * 1000) + " seconds";
        }
    }

    public static void close(Closeable c) {
        if (c != null) {
            try { c.close(); } catch (Throwable ignore) {}
        }
    }

    public static void post(Runnable r) {
        mainHandler.post(r);
    }

    public static void postDelayed(Runnable r, long milliseconds) {
        mainHandler.postDelayed(r, milliseconds);
    }

    public static int measure(int mode, int size, int preferred) {
        return mode == EXACTLY ? size :
              (mode == AT_MOST ? Math.min(preferred, size) : preferred);
    }

    public static void nanosleep(long nanoseconds) {
        try {
            long milliseconds = nanoseconds / 1000000;
            nanoseconds = nanoseconds % 1000000;
            Thread.sleep(milliseconds, (int)nanoseconds);
        } catch (InterruptedException ignore) {
            // just return
        }
    }

    private static String lineSeparator = System.getProperty("line.separator");
    private static int lineSeparatorChar = lineSeparator.charAt(0);

    public static void redirectSystemStreams() {
        if (lineSeparator.length() != 1) {
            throw new Error("it might not work on Windows CR/LF");
        }
        System.setOut(new LogPrintStream(new LogOutputStream(), System.out));
        System.setErr(new LogPrintStream(new LogOutputStream(), System.err));
//      System.err.println("System.err/out -> adb logcat");
    }

    private static class LogPrintStream extends PrintStream {

        private PrintStream second;

        public LogPrintStream(OutputStream out, PrintStream was) {
            super(out);
            second = was;
        }

        public void write(int ch)  {
            super.write(ch);
            second.write(ch);
        }

    }

    private static class LogOutputStream extends OutputStream {

        private CharArrayWriter saw = new CharArrayWriter(1024);

        public LogOutputStream() { }

        public void write(int ch) throws IOException {
            if (ch != lineSeparatorChar) {
                saw.write(ch);
            } else {
                int n = 4; // variable depth of calls inside java.io.* packages
                StackTraceElement[] s = Thread.currentThread().getStackTrace();
                for (int i = n; i < s.length; i++) {
                    if ("java.io".equals(forName(s[i-1].getClassName()).getPackage().getName()) &&
                       !"java.io".equals(forName(s[i].getClassName()).getPackage().getName())) {
                        n = i;
                        break;
                    }
                }
                StackTraceElement st = Thread.currentThread().getStackTrace()[n];
                Class cls = forName(st.getClassName());
                String pkg = cls.getPackage().getName();
                String caller = cls.getSimpleName() + "." + st.getMethodName() + ":" + st.getLineNumber() + " ";
                trace0(caller, pkg, saw.toString());
                saw.reset();
            }
        }

    }

}

package ip.c0d.ippg;

import android.app.*;
import android.content.*;
import android.content.res.*;
import android.os.*;

import jni.*;

import java.lang.*;

import static misc.util.*;

public class App extends Application {

    public static Activity act;

    static {
        redirectSystemStreams();
        System.loadLibrary("opencv_info");
        System.loadLibrary("opencv_java");
        System.loadLibrary("gnustl_shared");
        System.loadLibrary("yuv");
        System.loadLibrary("native");
        Android.clearMemoryLeaks();
    }

    public App() {
        G.app = this;
        trace("--- STARTING APP ---");
//      opencvInfo();
        registerComponentCallbacks(new ComponentCallbacks() {
            public void onConfigurationChanged(Configuration newConfig) { }
            public void onLowMemory() { }
        });
        registerActivityLifecycleCallbacks(new ActivityLifecycleCallbacks() {
            public void onActivityCreated(Activity a, Bundle savedInstanceState) { act = a; }
            public void onActivityStarted(Activity activity) { }
            public void onActivityResumed(Activity activity) { }
            public void onActivityPaused(Activity activity) { }
            public void onActivityStopped(Activity activity) { }
            public void onActivitySaveInstanceState(Activity activity, Bundle outState) { }
            public void onActivityDestroyed(Activity activity) {
                act = null;
            }
        });
    }

/*
    private void opencvInfo() {
        for (String s : Core.getBuildInformation().split("\n")) {
            trace(s);
        }
    }
*/

    public void onCreate() {
        super.onCreate();
    }

    public static class QuitReceiver extends BroadcastReceiver {

        public void onReceive(Context context, Intent intent) {
            if (act != null) {
                act.finish();
            }
        }

    }

    protected void finalize() throws Throwable {
        super.finalize();
        trace("");
    }

}

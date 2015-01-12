package ip.c0d.ippg;

import android.app.*;
import android.os.*;
import android.view.*;

public class BaseActivity extends Activity {

    protected View cv;

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    public void setContentView(View view) {
        cv = view;
        super.setContentView(view);
    }

    public void setContentView(View view, ViewGroup.LayoutParams params) {
        cv = view;
        super.setContentView(view, params);
    }

    public void addContentView(View view, ViewGroup.LayoutParams params) {
        throw new Error("not supported. Do not use");
    }

    public void onStart() {
        super.onStart();
        if (cv instanceof TopView) {
            ((TopView)cv).onStart();
        }
    }

    protected void onPause() {
        if (cv instanceof TopView) {
            ((TopView)cv).onPause();
        }
        super.onPause();
    }

    public void onDestroy() {
        if (cv instanceof TopView) {
            ((TopView)cv).onDestroy();
        }
        cv = null;
        super.onDestroy();
    }

    public void onResume() {
        if (cv instanceof TopView) {
            ((TopView)cv).onResume();
        }
        super.onResume();
    }

    public void onStop() {
        if (cv instanceof TopView) {
            ((TopView)cv).onStop();
        }
        super.onStop();
    }

    protected void onRestart() {
        if (cv instanceof TopView) {
            ((TopView)cv).onRestart();
        }
        super.onRestart();
    }

}

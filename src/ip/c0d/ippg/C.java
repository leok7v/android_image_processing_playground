package ip.c0d.ippg;

import android.util.*;
import android.view.*;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;

@SuppressWarnings({"unused"})
public class C {

    public static final int POINT = TypedValue.COMPLEX_UNIT_PT;
    public static final int PIXEL = TypedValue.COMPLEX_UNIT_PX;

    public static final int
            BITMAP_VIEW_BACKGROUND_COLOR = 0xFF2c5231,
            BACKGROUND_COLOR = 0xFF3A425E;

    public static final ViewGroup.LayoutParams
            WRAP_WRAP   =  new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT),
            MATCH_MATCH = new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT),
            MATCH_WRAP  = new ViewGroup.LayoutParams(MATCH_PARENT, WRAP_CONTENT),
            WRAP_MATCH  = new ViewGroup.LayoutParams(WRAP_CONTENT, MATCH_PARENT);

    public static final int FPS = 30;
    public static final int WIDTH = 640;
    public static final int HEIGHT = 480;
    public static final int BPP = 1;
}

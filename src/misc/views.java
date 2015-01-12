package misc;

import android.content.*;
import android.view.*;
import android.widget.*;

import static misc.util.assertion;

@SuppressWarnings("UnusedDeclaration")
public final class views {

    /**
     * @param c Context
     * @param orientation layout direction
     * @param params sequence of {view[,lp],view[,lp],view[,lp]...}
     * @return new LinearLayout with added children
     */
    public static LinearLayout createLinearLayout(Context c, int orientation, Object... params) {
        LinearLayout ll = new LinearLayout(c);
        ll.setOrientation(orientation);
        return (LinearLayout)addChildren(ll, params);
    }

    /**
     * @param c Context
     * @param params sequence of {view[,lp],view[,lp],view[,lp]...}
     * @return new horizontal LinearLayout with added children
     */
    public static LinearLayout createHorizontal(Context c, Object... params) {
        return createLinearLayout(c, LinearLayout.HORIZONTAL, params);
    }

    /**
     * @param c Context
     * @param params sequence of {view[,lp],view[,lp],view[,lp]...}
     * @return new vertical LinearLayout with added children
     */
    public static LinearLayout createVertical(Context c, Object... params) {
        return createLinearLayout(c, LinearLayout.VERTICAL, params);
    }

    /**
     * @param params sequence of {view[,lp][,weight],view[,lp][,weight],view[,lp][,weight]...}
     * @return the view group passed in with children added
     */
    public static ViewGroup addChildren(ViewGroup p, Object... params) {
        if (params != null) {
            int i = 0;
            while (i < params.length) {
                View v = params[i] instanceof View ? (View)params[i] : null;
                ViewGroup.LayoutParams lp = i + 1 < params.length && params[i + 1] instanceof ViewGroup.LayoutParams ?
                        (ViewGroup.LayoutParams)params[i + 1] : null;
                if (v == null) {
                    assertion(false, "expected parameter View got " + params[i].getClass());
                    return p;
                }
                if (lp != null) {
                    p.addView(v, lp);
                    i++;
                } else {
                    p.addView(v); // this will call p.generateDefaultLayoutParameters()
                }
                Float weight = i + 1 < params.length && params[i + 1] instanceof Float ?
                        (Float)params[i + 1] : null;
                if (weight != null && v.getLayoutParams() instanceof LinearLayout.LayoutParams) {
                    LinearLayout.LayoutParams llp = (LinearLayout.LayoutParams)v.getLayoutParams();
                    llp.weight = weight;
                    v.setLayoutParams(llp);
                    i++;
                }
                i++;
            }
        }
        return p;
    }

    private views() {}

}

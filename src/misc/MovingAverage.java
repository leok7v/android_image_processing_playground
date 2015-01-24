package misc;

public class MovingAverage {

    /* http://en.wikipedia.org/wiki/Moving_average#Simple_moving_average */
    public double simple;      /* valid after push = sum / length   */
    /* http://en.wikipedia.org/wiki/Moving_average#Cumulative_moving_average */
    public double last;        /* valid after moving_average_push()   */
    public double cumulative;  /* valid after moving_average_push()   */
    public double min;         /* valid after moving_average_minmax() */
    public double max;         /* valid after moving_average_minmax() */

    private final double[] a;
    private int in;
    private int length;
    private double sum;
    private double measurements;

    public MovingAverage(int n) {
        a = new double[n];
    }

    public void reset() {
        in = 0;
        sum = 0;
        last = 0;
        length = 0;
        cumulative = 0;
        simple = 0;
        measurements = 0;
    }

    public void push(double value) {
        final int n = a.length;
        last = value;
        final int next = length + 1;
        length = next < n ? next : n;
        sum += value - (length == n ? a[in] : 0);
        a[in] = value;
        in = (in + 1) % n;
        measurements += 1;
        cumulative = cumulative + (value - cumulative) / measurements;
        simple = sum / length;

    }

    public void minmax() { /* call to update min and max */
        if (length > 0) {
            double min = a[0];
            double max = min;
            final int n = length;
            for (int i = 1; i < n; i++) {
                final double ai = a[i];
                if (ai > max) {
                    max = ai;
                } else if (ai < min) {
                    min = ai;
                }
            }
            this.min = min;
            this.max = max;
        }
    }

}

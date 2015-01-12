#include "image_processing.h"
#ifdef __ARM_NEON__
#include <arm_neon.h>
#else
#pragma GCC error "must compile with -mfpu=neon"
#endif

#pragma GCC diagnostic warning "-Wall"
#pragma GCC diagnostic warning "-pedantic"
#pragma GCC diagnostic warning "-Wunused-but-set-variable"
#pragma GCC diagnostic warning "-Wunused-variable"

// https://gcc.gnu.org/onlinedocs/gcc/Inline.html
static int inline min(const int a, const int b)  { return a < b ? a : b; }
static int inline max(const int a, const int b)  { return a > b ? a : b; }

static void threshold8u_neon(byte* input, int stride, int w, int h, byte value, byte set, byte* output);
static void threshold8u_x4(byte* input, int stride, int w, int h, byte value, byte set, byte* output);
static void threshold8u_x1(byte* input, int stride, int w, int h, byte value, byte set, byte* output);

static void dilate8unz_r1_simple(byte* input, int stride, int w, int h, byte set, byte* output);
static void dilate8unz_unrolled(byte* input, int stride, int w, int h, int radius, byte set, byte* output);

void ip_threshold8u(void* input, int stride, int w, int h, unsigned char value, unsigned char set, void* output) {
    assertion(input == output ||
              (byte*)input + h * stride <= (byte*)output || (byte*)output + h * stride <= (byte*)input,
              "input=[%p..%p] and output=[%p..%p] should not overlap",
              input, (byte*)input + h * stride - 1, output, (byte*)output + h * stride - 1);
    assertion(1 <= value && value <= 253, "value=%d is not in [1..254] range");
    if (stride % 16 == 0 && w % 16 == 0) {
        threshold8u_neon(input, stride, w, h, value, set, output);
    } else if (stride % 4 == 0 && w % 4 == 0) {
        threshold8u_x4(input, stride, w, h, value, set, output);
    } else {
        threshold8u_x1(input, stride, w, h, value, set, output);
    }
}

void ip_dilate8unz(void* input, int stride, int w, int h, int radius, unsigned char set, void* output) {
    assertion(input == output ||
              (byte*)input + h * stride <= (byte*)output || (byte*)output + h * stride <= (byte*)input,
              "input=[%p..%p] and output=[%p..%p] should not overlap",
              input, (byte*)input + h * stride - 1, output, (byte*)output + h * stride - 1);
    assertion(1 <= radius && radius <= 254, "k=%d is out of [1..254] range");
    assertion(1 <= set && set <= 255, "value of \"set\"=%d is out of [1..255] range");
    if (radius == 1) {
        dilate8unz_r1_simple(input, stride, w, h, set, output);
    } else {
        dilate8unz_unrolled(input, stride, w, h, radius, set, output);
    }
}

static void threshold8u_x1(byte* input, int stride, int w, int h, byte value, byte set, byte* output) {
    // 640x480 0.000492 sec
    // opencv uses "vcgtq_u8" instead of "vcgteq_u8" (which would be more logical) thus we have to comply :(
    byte t[256] = {0};
    for (int i = value + 1; i < 256; i++) {
        t[i] = set;
    }
    const int N = 10;
    for (int k = 0; k <= N; k++) {
        byte* in  = (byte*)input;
        byte* out = (byte*)output;
        for (int y = 0; y < h; y++) {
            byte* p = in;
            byte* o = out;
            byte* e = p + w;
            while (p != e) { *o++ = t[*p++]; }
            in += stride;
            out += stride;
        }
    }
}

static void threshold8u_x4(byte* input, int stride, int w, int h, byte value, byte set, byte* output) {
    // 640x480 0.000379 sec
    assertion(w % 4 == 0, "expected w=%d to be divisible by 4", w);
    assertion(stride % 4 == 0, "expected stride=%d to be divisible by 4", stride);
    unsigned int t0[256];
    unsigned int t1[256];
    unsigned int t2[256];
    unsigned int t3[256];
    // opencv uses "vcgtq_u8" instead of "vcgteq_u8" (which would be more logical) thus we have to comply :(
    for (int i = 0; i < 256; i++) {
        t0[i] = i <= value ? 0 : set;
        t1[i] = t0[i] << 8;
        t2[i] = t1[i] << 8;
        t3[i] = t2[i] << 8;
    }
    int s4 = stride / 4;
    int w4 = w / 4;
    unsigned int* in = (unsigned int*)input;
    unsigned int* out = (unsigned int*)output;
    for (int y = 0; y < h; y++) {
        unsigned int* p = in;
        unsigned int* o = out;
        unsigned int* e = p + w4;
        while (p != e) {
            register unsigned int b4 = *p++;
            *o++ = t3[(b4 >> 24) & 0xFF] | t2[(b4 >> 16) & 0xFF] | t1[(b4 >>  8) & 0xFF] | t0[b4 & 0xFF];
        }
        in += s4;
        out += s4;
    }
}

static void threshold8u_neon(byte* input, int stride, int w, int h, byte value, byte set, byte* output) {
    // see: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0491h/BABEHCCG.html
    // best time seen 100 microseconds for 640x480
    // 640x480 0.000096 sec
    assertion(w % 16 == 0, "w=%d must be divisible by 16", w);
    uint8x16_t v_thresh = vdupq_n_u8((byte)value);
    uint8x16_t v_max = vdupq_n_u8(set);
    byte* p = (byte*)input;
    byte* o = (byte*)output;
    for (int y = 0; y < h; y++) {
        byte* s = p;
        byte* d = o;
        byte* e = s + w;
        while (s < e) {
            // opencv uses "vcgtq_u8" instead of "vcgteq_u8" (which would be more logical) thus we have to comply :(
            vst1q_u8(d, vandq_u8(vcgtq_u8(vld1q_u8(s), v_thresh), v_max));
            d += 16;
            s += 16;
        }
        p += stride;
        o += stride;
    }
}

static void dilate8unz_r1_simple(byte* input, int stride, int w, int h, byte set, byte* output) {
    // 640x480 0.001092 sec
    byte line[w];
    memset(line, 0, w);
    for (int y = 0; y < h; y++) {
        int row = y * stride;
        for (int x = 0; x < w; x++) {
            int pos = row + x;
            bool v = input[pos] != 0;
            output[pos] = 0;
            if (line[x] != 0 || (x > 0 && line[x - 1] != 0)) {
                output[pos] = set;
            }
            if (line[x] != 0) {
                line[x] = 0;
            }
            if (v) {
                if (y > 0) output[pos - stride] = set;
                if (x > 0) output[pos - 1] = set;
                line[x] = set;
                output[pos] = set;
            }
        }
    }
}

static void dilate8unz_unrolled(byte* input, int stride, int w, int h, int radius, byte set, byte* output) {
    // 640x480 0.003534 sec
    byte* img = input;
    byte* out = output;
    const int farthest = w + h;
    int md[h * stride]; // Manhattan Distance
    md[0] = img[0] != 0 ? 0 : farthest; // left top corner
    // first row:
    for (int x = 1; x < w; x++) {
        md[x] = img[x] != 0 ? 0 : min(farthest, md[x - 1] + 1);
    }
    int row = stride;
    for (int y = 1; y < h; y++) {
        md[row] = img[row] != 0 ? 0 : min(farthest, md[row - stride] + 1);
        int pos = row;
        for (int x = 1; x < w; x++) {
            pos++;
            md[pos] = img[pos] != 0 ? 0 : min(min(farthest, md[pos - stride] + 1), md[pos - 1] + 1);
        }
        row += stride;
    }
    row -= stride;
    // bottom right to top left:
    int pos = row + w - 1;
    out[pos] = md[pos] <= radius ? set : 0;
    for (int x = w - 2; x >= 0; x--) {
        pos--;
        int r = md[pos] = min(md[pos], md[pos + 1] + 1);
        out[pos] = r <= radius ? set : 0;
    }
    for (int y = h - 2; y >= 0; y--) {
        row -= stride;
        int pos = row + w - 1;
        int r = md[pos] = min(md[pos], md[pos + stride] + 1);
        out[pos] = r <= radius ? set : 0;
        for (int x = w - 2; x >= 0; x--) {
            pos--;
            int r = md[pos] = min(min(md[pos], md[pos + stride] + 1), md[pos + 1] + 1);
            out[pos] = r <= radius ? set : 0;
        }
    }
}


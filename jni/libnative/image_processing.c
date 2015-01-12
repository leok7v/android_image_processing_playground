#include "image_processing.h"
#ifdef __ARM_NEON__
#include <arm_neon.h>
#else
#pragma GCC error "must compile with -mfpu=neon"
#endif

#define PROFILE 1

#ifdef PROFILE
#   define begin static int64_t min_time; int64_t start_time = cputime();
#   define end(N) \
    { \
        uint64_t delta = cputime() - start_time; \
        min_time = min_time == 0 ? delta : min(min_time, delta); \
        trace("%0.06f sec", min_time / (double)NANOSECONDS_IN_SECOND / (N)); \
    }
#else
#   define begin
#   define end(N)
#endif // PROFILE

// https://gcc.gnu.org/onlinedocs/gcc/Inline.html
static int inline min(const int a, const int b)  { return a < b ? a : b; }
static int inline max(const int a, const int b)  { return a > b ? a : b; }

static void threshold8u_neon(byte* input, int stride, int w, int h, byte value, byte set, byte* output);
static void threshold8u_x4(byte* input, int stride, int w, int h, byte value, byte set, byte* output);
static void threshold8u_x1(byte* input, int stride, int w, int h, byte value, byte set, byte* output);

static void dilate8unz_r1_simple(byte* input, int stride, int w, int h, byte set, byte* output);
static void dilate8unz_r1_unrolled(byte* input, int stride, int w, int h, byte set, byte* output);
static void dilate8unz_simple(byte* input, int stride, int w, int h, int radius, byte set, byte* output);
static void dilate8unz_unrolled(byte* input, int stride, int w, int h, int radius, byte set, byte* output);

static void dump(const char* label, byte* img, int stride, int x, int y, int w, int h);

static void check(byte* input, int stride, int w, int h, byte* output, byte* verify) {
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (output[y * stride + x] != verify[y * stride + x]) {
                print_stacktrace();
                dump("input",  input,  stride, x, y, w, h);
                dump("output", output, stride, x, y, w, h);
                dump("verify", verify, stride, x, y, w, h);
                assertion(output[y * stride + x] == verify[y * stride + x], "output[y=%d, x=%d %d]=%d != verify[]=%d", y, x, y * stride + x, output[y * stride + x], verify[y * stride + x]);
            }
        }
    }
}

void ip_threshold8u(void* input, int stride, int w, int h, unsigned char value, unsigned char set, void* output) {
    assertion(input == output || input + h * stride <= output || output + h * stride <= input,
              "input=[%p..%p] and output=[%p..%p] should not overlap",
              input, input + h * stride -1, output, output + h * stride -1);
    byte verify[h * stride];
    threshold8u_x1(input, stride, w, h, value, set, verify);
    if (stride % 16 == 0 && w % 16 == 0) {
        threshold8u_neon(input, stride, w, h, value, set, output);
        check(input, stride, w, h, output, verify);
        assertion(memcmp(verify, output, h * stride) == 0, "threshold8u_neon != threshold8u_x1");
        threshold8u_x4(input, stride, w, h, value, set, output); // test
        check(input, stride, w, h, output, verify);
        assertion(memcmp(verify, output, h * stride) == 0, "threshold8u_x4 != threshold8u_x1");
    } else if (stride % 4 == 0 && w % 4 == 0) {
        threshold8u_x4(input, stride, w, h, value, set, output);
        check(input, stride, w, h, output, verify);
        assertion(memcmp(verify, output, h * stride) == 0, "threshold8u_x4 != threshold8u_x1");
    } else {
        threshold8u_x1(input, stride, w, h, value, set, output);
    }
}

void ip_dilate8unz(void* input, int stride, int w, int h, int radius, unsigned char set, void* output) {
    assertion(input == output || input + h * stride <= output || output + h * stride <= input,
              "input=[%p..%p] and output=[%p..%p] should not overlap",
              input, input + h * stride -1, output, output + h * stride -1);
    byte verify[h * stride];
    dilate8unz_simple(input, stride, w, h, radius, set, verify);
    if (radius == 1) {
        dilate8unz_r1_simple(input, stride, w, h, set, output);
        check(input, stride, w, h, output, verify);
        assertion(memcmp(verify, output, h * stride) == 0, "dilate8unz_r1_simple != dilate8unz_simple");
        dilate8unz_r1_unrolled(input, stride, w, h, set, output);
        check(input, stride, w, h, output, verify);
        assertion(memcmp(verify, output, h * stride) == 0, "dilate8unz_r1_unrolled != dilate8unz_simple");
        dilate8unz_unrolled(input, stride, w, h, radius, set, output);
        check(input, stride, w, h, output, verify);
        assertion(memcmp(verify, output, h * stride) == 0, "dilate8unz_unrolled != dilate8unz_simple");
    } else {
        dilate8unz_simple(input, stride, w, h, radius, set, output);
    }
}

static void threshold8u_x1(byte* input, int stride, int w, int h, byte value, byte set, byte* output) {
    // 0.000492 sec
    begin
    // opencv uses "vcgtq_u8" instead of "vcgteq_u8" (which would be more logical) thus we have to comply :(
    assertion(1 <= value && value <= 253, "value=%d is not in [1..254] range");
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
            while (p != e) { *o++ = t[*p++];
//              int x = (p - in);
//              assertion(verify == null || o[-1] == verify[y * stride + x], "o[y=%d, x=%d %d]=%d != out[]=%d", y, x, y * stride + x, o[-1], verify[y * stride + x]);
            }
            in += stride;
            out += stride;
        }
    }
    end(N);
}

static void threshold8u_x4(byte* input, int stride, int w, int h, byte value, byte set, byte* output) {
    begin // 0.000379 sec
    assertion(1 <= value && value <= 253, "value=%d is not in [1..254] range");
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
    const int N = 10;
    for (int k = 0; k <= N; k++) {
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
    end(N);
}

static void threshold8u_neon(byte* input, int stride, int w, int h, byte value, byte set, byte* output) {
    // see: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0491h/BABEHCCG.html
    // best time seen 100 microseconds for 640x480
    begin // 0.000096 sec
    assertion(w % 16 == 0, "w=%d must be divisible by 16", w);
    const int N = 10;
    for (int k = 0; k <= N; k++) {
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
/*
                for (int i = 0; i < 16; i++) {
                    int x = (s - p) + i;
                    assertion((input[y * stride + x] > value) == (output[y * stride + x] == set),
                               "input[y=%d, x=%d %d]=%3d output[]=%3d", y, x, y * stride + x, input[y * stride + x], output[y * stride + x]
                              );
                }
*/
                d += 16;
                s += 16;
            }
            p += stride;
            o += stride;
        }
    }
    end(N);
}

static void dilate8unz_r1_simple(byte* input, int stride, int w, int h, byte set, byte* output) {
    // 640x480 0.001092 sec
    assert(set != 0);
    begin
    byte line[w];
    memset(line, 0, w);
    for (int y = 0; y < h; y++) {
        int row = y * stride;
        for (int x = 0; x < w; x++) {
            int pos = row + x;
            bool v = input[pos] != 0;
            output[pos] = 0;
            if (line[x] != 0 || x > 0 && line[x - 1] != 0) {
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
    end(1);
}

static void dilate8unz_r1_unrolled(byte* input, int stride, int w, int h, byte set, byte* output) {
    // 640x480 0.001254 sec
    assert(set != 0);
    begin
    byte line[w];
    memset(line, 0, w);
    byte* or = output;
    byte* ir = input;
    byte* ie = input + (h - 1) * stride;
    line[0] = output[0] = input[0] != 0 ? set : 0;
    for (int x = 1; x < w; x++) {
        bool bit = ir[x] != 0;
        bool west  = line[x - 1] != 0;
        bool north = line[x] != 0;
        if (north) { line[x] = 0; }
        or[x] = north | west | bit ? set : 0;
        if (bit) {
            or[x - 1] = line[x] = set;
        }
    }
    while (ir < ie) {
        ir += stride;
        or += stride;
        bool bit = ir[0] != 0;
        bool north = line[0] != 0;
        if (north) { line[0] = 0; }
        or[0] = north | bit ? set : 0;
        if (bit) {
           or[0] = or[-stride] = line[0] = set;
        }
        for (int x = 1; x < w; x++) {
            bool bit = ir[x] != 0;
            bool north = line[x] != 0;
            if (north) { line[x] = 0; }
            bool west  = line[x - 1] != 0;
            or[x] = north | west | bit ? set : 0;
            if (bit) {
                or[x - 1] = or[x - stride] = line[x] = set;
            }
        }
    }
    end(1);
}

static void dilate8unz_unrolled(byte* input, int stride, int w, int h, int radius, byte set, byte* output) {
    begin // 0.003534 sec
    assertion(1 <= radius && radius <= 254, "k=%d is out of [1..254] range");
    assertion(1 <= set && set <= 255, "value of \"set\"=%d is out of [1..255] range");
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
    // last row:
    row -= stride;
    ////////////////////////////////////////////// bottom right to top left: ///////////////////////////////
    int pos = row + w - 1;
    out[pos] = md[pos] <= radius ? set : 0;
    for (int x = w - 2; x >= 0; x--) {
        pos--;
        int was = md[pos];
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
    end(1);
}

static void dilate8unz_simple(byte* input, int stride, int w, int h, int radius, byte set, byte* output) {
    begin // 0.006151 sec
    assertion(1 <= radius && radius <= 254, "k=%d is out of [1..254] range");
    assertion(1 <= set && set <= 255, "value of \"set\"=%d is out of [1..255] range");
    byte* img = input;
    byte* out = output;
    const int farthest = w + h;
    int md[h * stride]; // Manhattan Distance
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int v = img[i * stride + j] == 0 ? 0 : 1;
            if (v > 0) {
                md[i * stride + j] = 0;
            } else {
                md[i * stride + j] = farthest;
                if (i > 0) md[i * stride + j] = min(md[i * stride + j], md[(i - 1) * stride + j] + 1);
                if (j > 0) md[i * stride + j] = min(md[i * stride + j], md[i * stride + (j - 1)] + 1);
            }
        }
    }
    for (int i = h - 1; i >= 0; i--) {
        for (int j = w - 1; j >= 0; j--) {
            if (i + 1 < h) md[i * stride + j] = min(md[i * stride + j], md[(i + 1) * stride + j]+1);
            if (j + 1 < w) md[i * stride + j] = min(md[i * stride + j], md[i * stride + (j + 1)] + 1);
        }
    }
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            out[i * stride + j] = md[i * stride + j] <= radius ? set : 0;
        }
    }
    end(1);
}

static void dump(const char* label, byte* img, int stride, int x, int y, int w, int h) {
    char buf[512];
    char num[16];
    trace("%s y=%d x=%d offset=%d", label, y, x, y * stride + x);
    int x16 = x & ~0xF;
    sprintf(buf, "%4c", ' ');
    for (int j = max(0, x16 - 16); j < min(x16 + 16, w); j++) {
        sprintf(num, " %03d", j);
        strcat(buf, num);
    }
    trace(buf);
    for (int i = max(0, y - 2); i < min(y + 3, h); i++) {
        int x16 = x & ~0xF;
        sprintf(buf, "%3d:", i);
        for (int j = max(0, x16 - 16); j < min(x16 + 16, w); j++) {
            sprintf(num, "%c%03d", y == i && x == j ? '>' : ' ', img[i * stride + j]);
            strcat(buf, num);
        }
        trace(buf);
    }
}

/*
static void dump_md(const char* label, int* distance, int stride, int x, int y, int w, int h) {
    char buf[512];
    char num[16];
    trace("%s y=%d x=%d offset=%d", label, y, x, y * stride + x);
    int x16 = x & ~0xF;
    sprintf(buf, "%4c", ' ');
    for (int j = max(0, x16 - 16); j < min(x16 + 16, w); j++) {
        sprintf(num, " %03d", j);
        strcat(buf, num);
    }
    trace(buf);
    for (int i = max(0, y - 2); i < min(y + 3, h); i++) {
        int x16 = x & ~0xF;
        sprintf(buf, "%3d:", i);
        for (int j = max(0, x16 - 16); j < min(x16 + 16, w); j++) {
            sprintf(num, "%c%03d", y == i && x == j ? '>' : ' ', distance[i * stride + j]);
            strcat(buf, num);
        }
        trace(buf);
    }
}
*/


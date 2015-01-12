#include "image_processing.h"
#ifdef __ARM_NEON__
#include <arm_neon.h>
#else
#pragma GCC error "must compile with -mfpu=neon"
#endif

#define PROFILE 1

#ifdef PROFILE
#   define begin static int64_t min_time; int64_t start_time = cputime();
#   define end(s, N) \
    { \
        uint64_t delta = cputime() - start_time; \
        min_time = min_time == 0 ? delta : min(min_time, delta); \
        trace("%s=%0.06f sec", (s), min_time / (double)NANOSECONDS_IN_SECOND / (N)); \
    }
#else
#   define begin
#   define end(s, N)
#endif // PROFILE

// https://gcc.gnu.org/onlinedocs/gcc/Inline.html
static int inline min(const int a, const int b)  { return a < b ? a : b; }
static int inline max(const int a, const int b)  { return a > b ? a : b; }

static void threshold8u_neon(byte* input, int stride, int w, int h, byte value, byte set, byte* output, byte* verify);
static void threshold8u_x4(byte* input, int stride, int w, int h, byte value, byte set, byte* output);
static void threshold8u_x1(byte* input, int stride, int w, int h, byte value, byte set, byte* output, byte* verify);

static void dilate8unz_r1(byte* input, int stride, int w, int h, byte set, byte* output, byte* verify);
static void dilate8unz_simple(byte* input, int stride, int w, int h, int radius, byte set, byte* output, byte* verify);
static void dilate8unz_unrolled(byte* input, int stride, int w, int h, int radius, byte set, byte* output,
                           int* md1, int* md2, byte* verify);

static void dump(const char* label, byte* img, int stride, int x, int y, int w, int h);
static void dumd(const char* label, int* distance, int stride, int x, int y, int w, int h);

void ip_threshold8u(void* input, int stride, int w, int h, unsigned char value, unsigned char set, void* output) {
    byte local[h * stride];
    {
        threshold8u_neon(input, stride, w, h, value, set, local, null);
        threshold8u_x1(input, stride, w, h, value, set, output, local);
    }
    threshold8u_x1(input, stride, w, h, value, set, local, null);
    if (stride % 4 == 0 && w % 4 == 0) {
        byte local4[h * stride];
        threshold8u_x4(input, stride, w, h, value, set, local4);
        assertion(memcmp(local, local4, h * stride) == 0, "threshold8u_x4 != threshold8u_x1");
    }
    if (stride % 16 == 0 && w % 16 == 0) {
        threshold8u_neon(input, stride, w, h, value, set, output, local);
        assertion(memcmp(local, output, h * stride) == 0, "threshold8u_neon != threshold8u_x1");
    } else if (stride % 4 == 0 && w % 4 == 0) {
        threshold8u_x4(input, stride, w, h, value, set, output);
    } else {
        threshold8u_x1(input, stride, w, h, value, set, output, null);
    }
}

static void threshold8u_x1(byte* input, int stride, int w, int h, byte value, byte set, byte* output, byte* verify) {
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
        if (verify != null) {
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    if (output[y * stride + x] != verify[y * stride + x]) {
                        dump("input",  input,  stride, x, y, w, h);
                        dump("output", output, stride, x, y, w, h);
                        dump("verify", verify, stride, x, y, w, h);
                        assertion(output[y * stride + x] == verify[y * stride + x], "output[y=%d, x=%d %d]=%d != verify[]=%d", y, x, y * stride + x, output[y * stride + x], verify[y * stride + x]);
                    }

                }
            }
        }
    }
    end("min_threshold8u_x1", N);
}

static void threshold8u_x4(byte* input, int stride, int w, int h, byte value, byte set, byte* output) {
    begin
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
    end("min_threshold8u_x4", N);
}

static void threshold8u_neon(byte* input, int stride, int w, int h, byte value, byte set, byte* output, byte* verify) {
    // see: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0491h/BABEHCCG.html
    // best time seen 100 microseconds for 640x480
    begin
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
                for (int i = 0; i < 16; i++) {
                    int x = (s - p) + i;
                    if ((input[y * stride + x] > value) != (output[y * stride + x] == set)) {
                        dump("input",  input,  stride, x, y, w, h);
                        dump("output", output, stride, x, y, w, h);
                        dump("verify", verify, stride, x, y, w, h);
                    }
                    assertion((input[y * stride + x] > value) == (output[y * stride + x] == set),
                               "input[y=%d, x=%d %d]=%3d output[]=%3d verify[]=%3d", y, x, y * stride + x, input[y * stride + x], output[y * stride + x]
                              );
                }
                for (int i = 0; i < 16 && verify != null; i++) {
                    int x = (s - p) + i;
                    if (d[i] != verify[y * stride + x]) {
                        dump("input",  input,  stride, x, y, w, h);
                        dump("output", output, stride, x, y, w, h);
                        dump("verify", verify, stride, x, y, w, h);
                    }
                    assertion(d[i] == verify[y * stride + x], "d[y=%d, x=%d %d]=%d != verify[]=%d", y, x, y * stride + x, d[i], verify[y * stride + x]);
                }
                d += 16;
                s += 16;
            }
            p += stride;
            o += stride;
        }
    }
    end("min_threshold8u_neon", N);
}


void ip_dilate8unz(void* input, int stride, int w, int h, int radius, unsigned char set, void* output) {
    assertion(input == output || input + h * stride <= output || output + h * stride <= input,
              "input=[%p..%p] and output=[%p..%p] should not overlap",
              input, input + h * stride -1, output, output + h * stride -1);
    if (radius == 1) {
        dilate8unz_simple(input, stride, w, h, radius, set, output, null);
        byte local[h * stride];
        dilate8unz_r1(input, stride, w, h, set, local, output);
        assert(memcmp(local, output, h * stride) == 0);
    } else {
        dilate8unz_simple(input, stride, w, h, radius, set, output, null);
    }
}

static void dilate8unz_r1_0(byte* input, int stride, int w, int h, byte set, byte* output, byte* verify) {
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
    end("min_dilate_r1_0_time", 1);
    if (verify != null) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                if (output[y * stride + x] != verify[y * stride + x]) {
                    dump("input",  input,  stride, x, y, w, h);
                    dump("output", output, stride, x, y, w, h);
                    dump("verify", verify, stride, x, y, w, h);
                    assertion(output[y * stride + x] == verify[y * stride + x], "output[%d]=%d != verify[]=%d y=%d x=%d", y * stride + x, output[y * stride + x], verify[y * stride + x], y, x);
                }
            }
        }
    }
}

static void dilate8unz_r1(byte* input, int stride, int w, int h, byte set, byte* output, byte* verify) {
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
    end("min_dilate_r1_time", 1);
    if (verify != null) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                if (output[y * stride + x] != verify[y * stride + x]) {
                    dump("input",  input,  stride, x, y, w, h);
                    dump("output", output, stride, x, y, w, h);
                    dump("verify", verify, stride, x, y, w, h);
                    assertion(output[y * stride + x] == verify[y * stride + x], "output[%d]=%d != verify[]=%d y=%d x=%d", y * stride + x, output[y * stride + x], verify[y * stride + x], y, x);
                }
            }
        }
    }
    dilate8unz_r1_0(input, stride, w, h, set, verify, null);
    if (verify != null) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                if (output[y * stride + x] != verify[y * stride + x]) {
                    dump("input",  input,  stride, x, y, w, h);
                    dump("output", output, stride, x, y, w, h);
                    dump("verify", verify, stride, x, y, w, h);
                    assertion(output[y * stride + x] == verify[y * stride + x], "output[%d]=%d != verify[]=%d y=%d x=%d", y * stride + x, output[y * stride + x], verify[y * stride + x], y, x);
                }
            }
        }
    }
}


static void dilate8unz_unrolled(byte* input, int stride, int w, int h, int radius, byte set, byte* output,
                       int* md1, int* md2, byte* verify) {
    begin
    assertion(1 <= radius && radius <= 254, "k=%d is out of [1..254] range");
    assertion(1 <= set && set <= 255, "value of \"set\"=%d is out of [1..255] range");
    byte* img = input;
    byte* out = output;
    const int farthest = w + h;
    int md[h * stride]; // Manhattan Distance
    md[0] = img[0] != 0 ? 0 : farthest; // left top corner
    if (md1 != null) {
        assertion(md[0] == md1[0], "md[0]=%d != md1[0]=%d", md[0], md1[0]);
    }
    // first row:
    for (int x = 1; x < w; x++) {
           md[x] = img[x] != 0 ? 0 : min(farthest, md[x - 1] + 1);
        if (md1 != null) {
            assertion(md[x] == md1[x], "md[%d]=%d != md1[%d]=%d", x, md[x], x, md1[x]);
        }
    }
    int row = stride;
    for (int y = 1; y < h; y++) {
//      int row1 = y * stride;
//      assertion(row1 == row, "row1=%d row=%d y=%d", row1, row, y);
        md[row] = img[row] != 0 ? 0 : min(farthest, md[row - stride] + 1);
        if (md1 != null) {
            assertion(md[row] == md1[row], "md[%d]=%d != md1[%d]=%d", row, md[row], row, md1[row]);
        }
        int pos = row;
        for (int x = 1; x < w; x++) {
            pos++;
//          int pos1 = row + x;
//          assertion(pos1 == pos, "pos1=%d pos=%d x=%d y=%d", row1, row, x, y);
            md[pos] = img[pos] != 0 ? 0 : min(min(farthest, md[pos - stride] + 1), md[pos - 1] + 1);
            if (md1 != null) {
                assertion(md[pos] == md1[pos], "md[%d]=%d != md1[%d]=%d", pos, md[pos], pos, md1[pos]);
            }
        }
        row += stride;
    }
    assertion(row == h * stride, "raw=%d h * stride=%d", row, h * stride);
    // last row:
    row -= stride;
    assertion(row == (h - 1) * stride, "raw=%d (h - 1) * stride=%d", row, (h - 1) * stride);
    if (md1 != null) {
        assertion(md[(h - 1) * stride + w - 1] == md1[(h - 1) * stride + w - 1], "md[h - 1, w - 1]=%d md2=%d", md[(h - 1) * stride + w - 1], md2[(h - 1) * stride + w - 1]);
    }
    if (md2 != null) {
        assertion(md[(h - 1) * stride + w - 1] == md2[(h - 1) * stride + w - 1], "md[h - 1, w - 1]=%d md2=%d", md[(h - 1) * stride + w - 1], md2[(h - 1) * stride + w - 1]);
    }
    ////////////////////////////////////////////// bottom right to top left: ///////////////////////////////
    int pos = row + w - 1;
    out[pos] = md[pos] <= radius ? set : 0;
    assertion(pos == (h - 1) * stride + w - 1, "pos=%d ((h - 1) * stride + w - 1)=%d", pos, (h - 1) * stride + w - 1);
    if (verify != null) {
        assertion(out[row] == verify[row], "out[h - 1, w - 1]=%d verify[]=%d", out[row], verify[row]);
    }
    for (int x = w - 2; x >= 0; x--) {
        pos--;
        assertion(pos == (h - 1) * stride + x, "raw=%d (h - 1) * stride=%d row=%d x = %d", pos, (h - 1) * stride + x, row, x);
        int was = md[pos];
        int r = md[pos] = min(md[pos], md[pos + 1] + 1);
        out[pos] = r <= radius ? set : 0;
        if (md2 != null) {
            assertion(md[pos] == md2[pos], "md[%d]=%d != md2[%d]=%d x=%d was=%d md[pos + 1] + 1 = %d", pos, md[pos], pos, md2[pos], x, was, md[pos + 1] + 1);
        }
        if (verify != null) {
            if (out[pos] != verify[pos]) {
                dump("input",  input,  stride, x, h - 1, w, h);
                dump("output", output, stride, x, h - 1, w, h);
                dump("verify", verify, stride, x, h - 1, w, h);
                dumd("md",  md, stride, x, h - 1, w, h);
            }
            assertion(out[pos] == verify[pos], "out[%d]=%d verify[%d]=%d", pos, out[pos], pos, verify[pos]);
        }
    }
    for (int y = h - 2; y >= 0; y--) {
        row -= stride;
        int row1 = y * stride;
        assertion(row1 == row, "row1=%d row=%d y=%d", row1, row, y);
        int pos = row + w - 1;
        int r = md[pos] = min(md[pos], md[pos + stride] + 1);
        out[pos] = r <= radius ? set : 0;
        assertion(pos == y * stride + w - 1, "pos=%d (y * stride + w - 1)=%d y=%d", pos, y * stride + w - 1, y);
        if (md2 != null) {
            assertion(md[pos] == md2[pos], "md[%d]=%d != md2[%d]=%d y=%d", pos, md[row], pos, md2[row], y);
        }
        for (int x = w - 2; x >= 0; x--) {
            pos--;
            int pos1 = row + x;
            assertion(pos1 == pos, "pos1=%d pos=%d x=%d y=%d", row1, row, x, y);
            int r = md[pos] = min(min(md[pos], md[pos + stride] + 1), md[pos + 1] + 1);
            out[pos] = r <= radius ? set : 0;
            if (md2 != null) {
                assertion(md[pos] == md2[pos], "md[%d]=%d != md2[%d]=%d", pos, md[pos], pos, md2[pos]);
            }
            if (verify != null) {
                assertion(out[pos] == verify[pos], "out[%d]=%d verify[%d]=%d x=%d y=%d", pos, out[pos], pos, verify[row], x, y);
            }
        }
    }
    end("dilate8unz_unrolled", 1);
    if (verify != null) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                if (output[y * stride + x] != verify[y * stride + x]) {
                    dump("input",  input,  stride, x, y, w, h);
                    dump("output", output, stride, x, y, w, h);
                    dump("verify", verify, stride, x, y, w, h);
                    dumd("distance", md, stride, x, y, w, h);
                    assertion(output[y * stride + x] == verify[y * stride + x], "output[%d]=%d != verify[]=%d y=%d x=%d", y * stride + x, output[y * stride + x], verify[y * stride + x], y, x);
                }
            }
        }
    }
}

static void dilate8unz_simple(byte* input, int stride, int w, int h, int radius, byte set, byte* output, byte* verify) {
    assertion(1 <= radius && radius <= 254, "k=%d is out of [1..254] range");
    assertion(1 <= set && set <= 255, "value of \"set\"=%d is out of [1..255] range");
    begin
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
    byte local[h * stride]; // only for testing
//  dilate8unz_unrolled(input, stride, w, h, radius, set, local, md, null, null);
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
    end("dilate8unz_simple", 1);
    dilate8unz_unrolled(input, stride, w, h, radius, set, local, null, md, null);
    dilate8unz_unrolled(input, stride, w, h, radius, set, local, null, null, verify);

    if (verify != null) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                if (output[y * stride + x] != verify[y * stride + x]) {
                    dump("input",  input,  stride, x, y, w, h);
                    dump("output", output, stride, x, y, w, h);
                    dump("verify", verify, stride, x, y, w, h);
                    dumd("distance", md, stride, x, y, w, h);
                    assertion(output[y * stride + x] == verify[y * stride + x], "output[%d]=%d != verify[]=%d y=%d x=%d md[]=%d", y * stride + x, output[y * stride + x], verify[y * stride + x], y, x, md[y * stride + x]);
                }
            }
        }
    }
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

static void dumd(const char* label, int* distance, int stride, int x, int y, int w, int h) {
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


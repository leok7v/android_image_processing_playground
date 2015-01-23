#include "image_processing.h"
#include <sys/endian.h>
#ifdef __ARM_NEON__
#include <arm_neon.h>
#else
#pragma GCC error "must compile with -mfpu=neon"
#endif

enum { PRE_ALLOCATED_BLOBS = 100, PRE_ALLOCATED_SEGMENTS = 10000 };

#if defined(_BYTE_ORDER) & defined(_LITTLE_ENDIAN)
#   define IS_HOST_LITTLE_ENDIAN (_BYTE_ORDER == _LITTLE_ENDIAN)
#else
static const union { unsigned char bytes[4]; uint32_t value; } endianness = { { 0, 1, 2, 3 } };
#   define IS_HOST_LITTLE_ENDIAN (endianness.value == 0x03020100ul)
#endif

#pragma GCC diagnostic warning "-Wall"
#pragma GCC diagnostic warning "-pedantic"
#pragma GCC diagnostic warning "-Wunused-but-set-variable"
#pragma GCC diagnostic warning "-Wunused-variable"

ip_context_t* ip_create(int stride, int w, int h) {
    assertion(0 < w && 0 < h && w <= stride, "invalid value for w=%d h=%d or stride=%d", w, h, stride);
    ip_context_t* context = (ip_context_t*)calloc(1, sizeof(ip_context_t));
    if (context != null) {
        assertion(sizeof(context->btt) == 256, "invalid size of btt=%d", sizeof(context->btt));
        context->stride = stride;
        context->w = w;
        context->h = h;
        context->non_zero.left = 0;
        context->non_zero.top = 0;
        context->non_zero.right = w;
        context->non_zero.bottom = h;
        context->blobs_allocated_bytes = PRE_ALLOCATED_BLOBS * sizeof(ip_blob_t);
        context->segments_allocated_bytes = PRE_ALLOCATED_BLOBS * sizeof(ip_point_t);
        context->blobs = malloc(context->blobs_allocated_bytes);
        context->segments = malloc(context->segments_allocated_bytes);
    }
    if (context->blobs == null || context->segments == null) {
        ip_destroy(context);
        context = null;
    }
    trace("ip_create %p", context);
    return context;
}

void ip_destroy(ip_context_t* context) {
    if (context != null) {
        free(context->blobs);
        free(context->segments);
        free(context);
    }
}

// https://gcc.gnu.org/onlinedocs/gcc/Inline.html
static int inline min(const int a, const int b)  { return a < b ? a : b; }
static int inline max(const int a, const int b)  { return a > b ? a : b; }

static void threshold_x1(ip_context_t* context, const byte const* input,
                         const byte threshold, const byte set, byte* output);
static void threshold_x4(ip_context_t* context, const byte const* input,
                         const byte threshold, const byte set, byte* output);
static void threshold_neon(ip_context_t* context, const byte const* input,
                           const byte threshold, const byte set, byte* output);

static void dilate_r1_simple(const byte const* input, const int stride, const int w, const int h,
                             const byte set, byte* output);

static void dilate_unrolled(const byte const* input, const int stride, const int w, const int h,
                            const int radius, const byte set, int* md, byte* output);

static void ip_inflate_rect(ip_rect_t* r, int dx, int dy, int min_x, int min_y, int max_x, int max_y);


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

static void check(byte* input, int stride, int w, int h, byte* output, byte* verify) {
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (output[y * stride + x] != verify[y * stride + x]) {
                print_stacktrace();
                dump("input",  input,  stride, x, y, w, h);
                dump("output", output, stride, x, y, w, h);
                dump("verify", verify, stride, x, y, w, h);
                assertion(output[y * stride + x] == verify[y * stride + x],
                          "output[y=%d, x=%d %d]=%d != verify[]=%d",
                          y, x, y * stride + x, output[y * stride + x], verify[y * stride + x]);
            }
        }
    }
}

#undef COMPARE_RESULTS

void ip_threshold(ip_context_t* context, void* input, unsigned char threshold, unsigned char set, void* output) {
    int64_t start_time = cputime();
    const int w = context->w;
    const int h = context->h;
    const int stride = context->stride;
    assertion(input == output ||
              (byte*)input + h * stride <= (byte*)output || (byte*)output + h * stride <= (byte*)input,
              "input=[%p..%p] and output=[%p..%p] should not overlap",
              input, (byte*)input + h * stride - 1, output, (byte*)output + h * stride - 1);
    assertion(1 <= threshold && threshold <= 253, "threshold=%d is not in [1..254] range");
    if (stride % 16 == 0 && w % 16 == 0 && (uintptr_t)input % 16 == 0 && (uintptr_t)output % 16 == 0) {
        threshold_neon(context, input, threshold, set, output);
    } else if (stride % 4 == 0 && w % 4 == 0 && (uintptr_t)input % 4 == 4 && (uintptr_t)output % 4 == 4) {
        trace("warning not 16 bytes aligned threshold %p %p %dx%d", input, output, w, h);
        threshold_x4(context, input, threshold, set, output);
    } else {
        trace("warning not 4 bytes aligned threshold %p %p %dx%d", input, output, w, h);
        threshold_x1(context, input, threshold, set, output);
    }
#ifdef COMPARE_RESULTS
    byte output1[w * h];
    timestamp("x1");
    threshold_x1(context, input, threshold, set, output1);
    timestamp("x1");
    ip_rect_t r1 = context->non_zero;

    byte output2[w * h];
    timestamp("x4");
    threshold_x4(context, input, threshold, set, output2);
    timestamp("x4");
    ip_rect_t r2 = context->non_zero;
    check(input, stride, w, h, output2, output1);
    assert(memcmp(output1, output2, w * h) == 0);
    assert(memcmp(&r1, &r2, sizeof(r1)) == 0);

    byte output3[w * h];
    timestamp("neon");
    threshold_neon(context, input, threshold, set, output3);
    timestamp("neon");
    ip_rect_t r3 = context->non_zero;
    check(input, stride, w, h, output3, output1);
    assert(memcmp(output1, output3, w * h) == 0);
    assert(memcmp(&r1, &r3, sizeof(r1)) == 0);
#endif
    context->threshold_time = (cputime() - start_time) / (double)NANOSECONDS_IN_SECOND;
}

void ip_dilate(ip_context_t* context, void* input, int radius, unsigned char set, void* output) {
    int64_t start_time = cputime();
    const int w = context->w;
    const int h = context->h;
    const int stride = context->stride;
    assertion(input == output ||
              (byte*)input + h * stride <= (byte*)output || (byte*)output + h * stride <= (byte*)input,
              "input=[%p..%p] and output=[%p..%p] should not overlap",
              input, (byte*)input + h * stride - 1, output, (byte*)output + h * stride - 1);
    assertion(1 <= radius && radius <= 254, "k=%d is out of [1..254] range");
    assertion(1 <= set && set <= 255, "threshold of \"set\"=%d is out of [1..255] range");
    ip_rect_t r = context->non_zero;
    const int nzw = r.right - r.left;
    const int nzh = r.bottom - r.top;
    if (nzw <= 0 || nzh <= 0) { return; } // all black
    ip_inflate_rect(&r, radius + 1, radius + 1, 0, 0, w, h);
    const int offset = r.top * stride + r.left;
    const byte const* in  = (const byte const*)input + offset;
    byte* out = (byte*)output + offset;
    if (radius == 1) {
        dilate_r1_simple(in, stride, nzw, nzh, set, out);
    } else {
        int md[w * h];
        dilate_unrolled(in, stride, nzw, nzh, radius, set, md, out);
    }
    context->dilate_time = (cputime() - start_time) / (double)NANOSECONDS_IN_SECOND;
}

static void ip_inflate_rect(ip_rect_t* r, int dx, int dy, int min_x, int min_y, int max_x, int max_y) {
    r->left = max(r->left - dx, min_x);
    r->top = max(r->top - dx, min_y);
    r->right = min(r->right + dy, max_x);
    r->bottom = min(r->bottom + dy, max_y);
}

static void non_zero_rectangle(ip_context_t* context, const uint32_t const* vnz, const void const* hnz, bool neon) {
    if (neon) {
        const byte const* hs = (const byte* const)hnz; // horizontal non zero bytes present
        const int w = context->w;
        int min_x = 0;
        while (min_x < w && hs[min_x] == 0) { min_x++; }
        int max_x = w - 1;
        while (max_x > 0 && hs[max_x] == 0) { max_x--; }
        context->non_zero.left = min_x;
        context->non_zero.right = max_x + 1;
    } else {
        const int const* hs = hnz;
        const int w = context->w;
        int min_x = 0;
        while (min_x < w && hs[min_x] == 0) { min_x++; }
        int max_x = w - 1;
        while (max_x > 0 && hs[max_x] == 0) { max_x--; }
        context->non_zero.left = min_x;
        context->non_zero.right = max_x + 1;
    }
    {
        const uint32_t const* vs = vnz;  // vertical non zero bytes present
        const int h = context->h;
        int min_y = 0;
        while (min_y < h && vs[min_y] == 0) { min_y++; }
        int max_y = h - 1;
        while (max_y > 0 && vs[max_y] == 0) { max_y--; }
        context->non_zero.top = min_y;
        context->non_zero.bottom = max_y + 1;
    }
    // the check below is not exactly necessary but practically saves a lot of grief
    if (context->non_zero.left >= context->non_zero.right ||
        context->non_zero.top  >= context->non_zero.bottom) {
        context->non_zero.left = 0;
        context->non_zero.right = 0;
        context->non_zero.top = 0;
        context->non_zero.bottom = 0;
    }
/*
    trace("context->non_zero %d,%d %dx%d",
           context->non_zero.left, context->non_zero.top,
           context->non_zero.right - context->non_zero.left,
           context->non_zero.bottom - context->non_zero.top);
*/
}

static void threshold_x1(ip_context_t* context, const byte const* input,
                         const byte threshold, const byte set, byte* output) {
    // 640x480 0.000883 sec
    // opencv uses "vcgtq_u8" instead of "vcgteq_u8" (which would be more logical) thus we have to comply :(
    const int w = context->w;
    const int h = context->h;
    const int stride = context->stride;
    if (context->threshold != threshold) {
        memset(context->btt, 0, sizeof(context->btt));
        for (int i = threshold + 1; i < 256; i++) {
            context->btt[i] = set;
        }
        context->threshold = threshold;
    }
    const byte const* btt = context->btt;
    uint32_t hnz[w];
    uint32_t vnz[h];
    memset(hnz, 0, sizeof(hnz));
    byte* in  = (byte*)input;
    byte* out = (byte*)output;
    for (int y = 0; y < h; y++) {
        unsigned int s = 0;
        for (int x = 0; x < w; x++) {
            byte v = btt[in[x]];
            s |= v;
            hnz[x] |= v;
            out[x] = v;
        }
        vnz[y] = s;
        in += stride;
        out += stride;
    }
    non_zero_rectangle(context, vnz, hnz, false);
}

static inline uint32_t clip(const uint32_t v, const uint32_t shift, const byte threshold, const byte set) {
    return (((v >> shift) & 0xFFU) > threshold ? 0xFFU : 0) & set;
}

static void threshold_x4(ip_context_t* context, const byte const* input,
                         const byte threshold, const byte set, byte* output) {
    // 640x480 0.000742 sec
    const int w = context->w;
    const int h = context->h;
    const int stride = context->stride;
    assertion(w % 4 == 0, "expected w=%d to be divisible by 4", w);
    assertion(stride % 4 == 0, "expected stride=%d to be divisible by 4", stride);
    assertion((uintptr_t)input  % 4 == 0, "expected input=%p to be divisible by 4", input);
    assertion((uintptr_t)output % 4 == 0, "expected output=%p to be divisible by 4", output);
    uint32_t vnz[h];
    uint32_t hnz[w];
    memset(hnz, 0, sizeof(hnz));
    const int s4 = stride / 4;
    const int w4 = w / 4;
    uint32_t* in  = (uint32_t*)input;
    uint32_t* out = (uint32_t*)output;
    if (IS_HOST_LITTLE_ENDIAN) {
        for (int y = 0; y < h; y++) {
            uint32_t* p = in;
            uint32_t* o = out;
            uint32_t* e = p + w4;
            uint32_t* hs = hnz;
            uint32_t  s = 0;
            while (p != e) {
                unsigned int b4 = *p++;
                unsigned int t3 = clip(b4, 24U, threshold, set);
                unsigned int t2 = clip(b4, 16U, threshold, set);
                unsigned int t1 = clip(b4,  8U, threshold, set);
                unsigned int t0 = clip(b4,  0U, threshold, set);
                s |= t3 | t2 | t1 | t0;
                *o++   = t0 | (t1 << 8U) | (t2 << 16U) | (t3 << 24U);
                *hs++ |= t0;
                *hs++ |= t1;
                *hs++ |= t2;
                *hs++ |= t3;
            }
            vnz[y] = s;
            in += s4;
            out += s4;
        }
    } else {
        for (int y = 0; y < h; y++) {
            uint32_t* p = in;
            uint32_t* o = out;
            uint32_t* e = p + w4;
            uint32_t* hs = hnz;
            unsigned int s = 0;
            while (p != e) {
                unsigned int b4 = *p++;
                unsigned int t3 = clip(b4, 24U, threshold, set);
                unsigned int t2 = clip(b4, 16U, threshold, set);
                unsigned int t1 = clip(b4,  8U, threshold, set);
                unsigned int t0 = clip(b4,  0U, threshold, set);
                s |= t3 | t2 | t1 | t0;
                *o++ = t3 | (t2 << 8U) | (t1 << 16U) | (t0 << 24U);
                *hs++ |= t3;
                *hs++ |= t2;
                *hs++ |= t1;
                *hs++ |= t0;
            }
            vnz[y] = s;
            in += s4;
            out += s4;
        }
    }
    non_zero_rectangle(context, vnz, hnz, false);
}

static void threshold_neon(ip_context_t* context, const byte const* input,
                           const byte threshold, const byte set, byte* output) {
    // see: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0491h/BABEHCCG.html
    // 640x480 = 307,200 min seen: 0.000265000 sec  < 1 nanosecond per pixel
    const int w = context->w;
    const int h = context->h;
    const int stride = context->stride;
    assertion(w % 16 == 0, "w=%d must be divisible by 16", w);
    uint8x16_t v_thresh = vdupq_n_u8((byte)threshold);
    uint8x16_t v_max = vdupq_n_u8(set);
    byte* p = (byte*)input;
    byte* o = (byte*)output;
    uint32_t vnz[h];
    byte hnz[w];
    memset(hnz, 0, sizeof(hnz));
    for (int y = 0; y < h; y++) {
        byte* s = p;
        byte* d = o;
        byte* e = s + w;
        byte* hs = hnz;
        int sum = 0;
        uint8x16_t next_b16 = vld1q_u8(s);
        uint8x16_t next_hs  = vld1q_u8(hs);
        while (s < e) {
            // opencv uses "vcgtq_u8" instead of "vcgteq_u8" (which would be more logical) thus we have to comply :(
            const uint8x16_t b16 = next_b16;
            const uint8x16_t h   = next_hs;
            if (s < e - 16) { next_b16 = vld1q_u8(s + 16); }
            const uint8x16_t r = vandq_u8(vcgtq_u8(b16, v_thresh), v_max);
            if (s < e - 16) { next_hs = vld1q_u8(hs + 16); }
            vst1q_u8(d, r);
            vst1q_u8(hs, vorrq_u8(h, r));
            if (sum == 0) {
                uint16x8_t  s1 = vpaddlq_u8(r);
                uint32x4_t  s2 = vpaddlq_u16(s1);
                uint64x2_t  s3 = vpaddlq_u32(s2);
                sum = vgetq_lane_u64(s3, 0) | vgetq_lane_u64(s3, 1);
            }
            d += 16;
            s += 16;
            hs += 16;
        }
        vnz[y] = sum;
        p += stride;
        o += stride;
    }
    non_zero_rectangle(context, vnz, hnz, true);
}

static void dilate_r1_simple(const byte const* input, const int stride, const int w, const int h, const byte set, byte* output) {
    // 640x480 0.001092 sec
    byte line[w];
    memset(line, 0, sizeof(line));
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

static void dilate_unrolled(const byte const* input, const int stride, const int w, const int h,
                            const int radius, const byte set, int* md, byte* output) {
    // 640x480 0.003534 sec
    const int farthest = w + h;
    md[0] = input[0] != 0 ? 0 : farthest; // Manhattan Distance: left top corner
    // first row:
    for (int x = 1; x < w; x++) {
        md[x] = input[x] != 0 ? 0 : min(farthest, md[x - 1] + 1);
    }
    int row = stride;
    for (int y = 1; y < h; y++) {
        md[row] = input[row] != 0 ? 0 : min(farthest, md[row - stride] + 1);
        int pos = row;
        for (int x = 1; x < w; x++) {
            pos++;
            md[pos] = input[pos] != 0 ? 0 : min(min(farthest, md[pos - stride] + 1), md[pos - 1] + 1);
        }
        row += stride;
    }
    row -= stride;
    // bottom right to top left:
    int pos = row + w - 1;
    output[pos] = md[pos] <= radius ? set : 0;
    for (int x = w - 2; x >= 0; x--) {
        pos--;
        int r = md[pos] = min(md[pos], md[pos + 1] + 1);
        output[pos] = r <= radius ? set : 0;
    }
    for (int y = h - 2; y >= 0; y--) {
        row -= stride;
        int pos = row + w - 1;
        int r = md[pos] = min(md[pos], md[pos + stride] + 1);
        output[pos] = r <= radius ? set : 0;
        for (int x = w - 2; x >= 0; x--) {
            pos--;
            int r = md[pos] = min(min(md[pos], md[pos + stride] + 1), md[pos + 1] + 1);
            output[pos] = r <= radius ? set : 0;
        }
    }
}


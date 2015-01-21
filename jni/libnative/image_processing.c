#include "image_processing.h"
#ifdef __ARM_NEON__
#include <arm_neon.h>
#else
#pragma GCC error "must compile with -mfpu=neon"
#endif

enum { PRE_ALLOCATED_BLOBS = 100, PRE_ALLOCATED_SEGMENTS = 10000 };

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
        context->vs = (int*)malloc(h * sizeof(int));
        context->hs = (int*)malloc(w * sizeof(int));
        context->md = (int*)malloc(w * h * sizeof(int));
        context->blobs_allocated_bytes = PRE_ALLOCATED_BLOBS * sizeof(ip_blob_t);
        context->segments_allocated_bytes = PRE_ALLOCATED_BLOBS * sizeof(ip_point_t);
        context->blobs = malloc(context->blobs_allocated_bytes);
        context->segments = malloc(context->segments_allocated_bytes);
    }
    if (context->vs == null || context->hs == null ||
        context->md == null ||
        context->blobs == null || context->segments == null) {
        ip_destroy(context);
        context = null;
    }
    trace("ip_create %p", context);
    return context;
}

void ip_destroy(ip_context_t* context) {
    if (context != null) {
        free(context->vs);
        free(context->hs);
        free(context->md);
        free(context->blobs);
        free(context->segments);
        free(context);
    }
}

// https://gcc.gnu.org/onlinedocs/gcc/Inline.html
static int inline min(const int a, const int b)  { return a < b ? a : b; }
static int inline max(const int a, const int b)  { return a > b ? a : b; }

static void threshold_nz(ip_context_t* context, const byte const* input, const byte threshold, const byte set, byte* output);
#if 0
static void threshold_neon(const byte const* input, const int stride, const int w, const int h, const byte threshold, const byte set, byte* output);
static void threshold_x4(const byte const* input, const int stride, const int w, const int h, const byte threshold, const byte set, byte* output);
static void threshold_x1(const byte const* input, const int stride, const int w, const int h, const byte threshold, const byte set, byte* output);
#endif

static void dilate_r1_simple(ip_context_t* context, const byte const* input, const byte set, byte* output);
static void dilate_unrolled(const byte const* input, const int stride, const int w, const int h, const int radius, const byte set, byte* output);
static void ip_inflate_rect(ip_rect_t* r, int dx, int dy, int min_x, int min_y, int max_x, int max_y);

void ip_threshold(ip_context_t* context, void* input, unsigned char threshold, unsigned char set, void* output) {
    int64_t start_time = cputime();
    const int h = context->h;
    const int stride = context->stride;
    assertion(input == output ||
              (byte*)input + h * stride <= (byte*)output || (byte*)output + h * stride <= (byte*)input,
              "input=[%p..%p] and output=[%p..%p] should not overlap",
              input, (byte*)input + h * stride - 1, output, (byte*)output + h * stride - 1);
    assertion(1 <= threshold && threshold <= 253, "threshold=%d is not in [1..254] range");
/*
    if (stride % 16 == 0 && w % 16 == 0) {
        threshold_neon(input, stride, w, h, threshold, set, output);
    } else if (stride % 4 == 0 && w % 4 == 0) {
        threshold_x4(input, stride, w, h, threshold, set, output);
    } else {
        threshold_x1(input, stride, w, h, threshold, set, output);
    }
*/
    threshold_nz(context, input, threshold, set, output);
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
    if (radius == 1) {
        dilate_r1_simple(context, input, set, output);
    } else {
        dilate_unrolled(input, stride, w, h, radius, set, output);
    }
    context->dilate_time = (cputime() - start_time) / (double)NANOSECONDS_IN_SECOND;
}

static void ip_inflate_rect(ip_rect_t* r, int dx, int dy, int min_x, int min_y, int max_x, int max_y) {
    r->left = max(r->left - dx, min_x);
    r->top = max(r->top - dx, min_y);
    r->right = min(r->right + dy, max_x);
    r->bottom = min(r->bottom + dy, max_y);
}

static void threshold_nz(ip_context_t* context, const byte const* input, const byte threshold, const byte set, byte* output) {
    // 640x480 0.0008 sec
    // opencv uses "vcgtq_u8" instead of "vcgteq_u8" (which would be more logical) thus we have to comply :(
    const int w = context->w;
    const int h = context->h;
    const int stride = context->stride;
    if (context->threshold != threshold) {
        memset(context->btt, 0, 256);
        for (int i = threshold + 1; i < 256; i++) {
            context->btt[i] = set;
        }
        context->threshold = threshold;
    }
    const byte const* btt = context->btt;
    int* hs = context->hs;
    int* vs = context->vs;
    memset(hs, 0, w * sizeof(int));
    byte* in  = (byte*)input;
    byte* out = (byte*)output;
    for (int y = 0; y < h; y++) {
        int s = 0;
        for (int x = 0; x < w; x++) {
            byte v = btt[in[x]];
            s += v;
            hs[x] += v;
            out[x] = v;
        }
        vs[y] = s;
        in += stride;
        out += stride;
    }
    int min_x = 0;
    while (min_x < w && hs[min_x] == 0) { min_x++; }
    int max_x = w - 1;
    while (max_x > 0 && hs[max_x] == 0) { max_x--; }
    context->non_zero.left = min_x;
    context->non_zero.right = max_x + 1;
    int min_y = 0;
    while (min_y < h && vs[min_y] == 0) { min_y++; }
    int max_y = h - 1;
    while (max_y > 0 && vs[max_y] == 0) { max_y--; }
    context->non_zero.top = min_y;
    context->non_zero.bottom = max_y + 1;
/*
    trace("context->non_zero %d,%d %dx%d",
           context->non_zero.left, context->non_zero.top,
           context->non_zero.right - context->non_zero.left,
           context->non_zero.bottom - context->non_zero.top);
*/
}

#if 0

static void threshold_x1(const byte const* input, const int stride, const int w, const int h, const byte threshold, const byte set, byte* output) {
    // 640x480 0.000492 sec
    // opencv uses "vcgtq_u8" instead of "vcgteq_u8" (which would be more logical) thus we have to comply :(
    byte t[256] = {0};
    for (int i = threshold + 1; i < 256; i++) {
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

static void threshold_x4(const byte const * input, const int stride, const int w, const int h, const byte threshold, const byte set, byte* output) {
    // 640x480 0.000379 sec
    assertion(w % 4 == 0, "expected w=%d to be divisible by 4", w);
    assertion(stride % 4 == 0, "expected stride=%d to be divisible by 4", stride);
    unsigned int t0[256];
    unsigned int t1[256];
    unsigned int t2[256];
    unsigned int t3[256];
    // opencv uses "vcgtq_u8" instead of "vcgteq_u8" (which would be more logical) thus we have to comply :(
    for (int i = 0; i < 256; i++) {
        t0[i] = i <= threshold ? 0 : set;
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

static void threshold_neon(const byte const * input, const int stride, const int w, const int h, const byte threshold, const byte set, byte* output) {
    // see: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0491h/BABEHCCG.html
    // best time seen 100 microseconds for 640x480
    // 640x480 0.000096 sec
    assertion(w % 16 == 0, "w=%d must be divisible by 16", w);
    uint8x16_t v_thresh = vdupq_n_u8((byte)threshold);
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

#endif

static void dilate_r1_simple(ip_context_t* context, const byte const* input, const byte set, byte * output) {
    // 640x480 0.001092 sec
    ip_rect_t r = context->non_zero;
    if (r.right <= r.left || r.bottom <= r.top) {
        return; // all black
    }
    const int w = context->w;
    const int h = context->h;
    const int stride = context->stride;
    ip_inflate_rect(&r, +2, +2, 0, 0, w, h);
    byte line[w];
    memset(line, 0, w);
    const int yf = r.top;
    const int yt = r.bottom;
    const int xf = r.left;
    const int xt = r.right;
    for (int y = yf; y < yt; y++) {
        int row = y * stride;
        for (int x = xf; x < xt; x++) {
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

static void dilate_unrolled(const byte const * input, const int stride, const int w, const int h, const int radius, const byte set, byte* output) {
    // 640x480 0.003534 sec
    // TODO: take non_zero rectangle into account
    const int farthest = w + h;
    int md[h * stride]; // Manhattan Distance
    md[0] = input[0] != 0 ? 0 : farthest; // left top corner
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


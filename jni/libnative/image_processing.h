#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H
#include "android.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ip_point_s {
    int x;
    int y;
} ip_point_t;

typedef struct ip_rect_s { /* right, bottom exclusive */
    int left;
    int top;
    int right;
    int bottom;
    /* pt(x,y) contains inside rectangle means left <= x < right && top <= y < bottom */
} ip_rect_t;

typedef struct ip_blob_s {
    int number_of_segments;
    int segments_start; /* ip_context.segments[segments_start .. segments_start + number_of_segments - 1] */
    float area; /* area estimate of the blob */
    float arc_length; /* perimeter */
    float roundness;  /* 1.0 round, 0 not even close */
    float center_x;   /* mass center of blob */
    float center_y;
    ip_rect_t bounding_rect;
} ip_blob_t;

typedef struct ip_context_s {
    int stride;
    int w;
    int h;
    /* output of ip_threshold */
    int threshold; /* last threshold "btt" was initialized for */
    unsigned char btt[256]; /* byte threshold table */
    ip_rect_t non_zero;    /* minimum output region that has non-zero pixels */
    int non_zero_x_ranges; /* <= w / 2 */
    int* non_zero_min_x;   /* [non_zero_x_ranges] */
    int* non_zero_max_x;   /* [non_zero_x_ranges] */
    int non_zero_y_ranges; /* <= h / 2 */
    int* non_zero_min_y;   /* [non_zero_y_ranges] */
    int* non_zero_max_y;   /* [non_zero_y_ranges] */
    /* output of ip_find_blobs */
    int number_of_blobs;
    int number_of_points;  /* sum of number of points in all segments */
    ip_blob_t*  blobs;
    ip_point_t* segments;
    int blobs_allocated_bytes;
    int segments_allocated_bytes;
    /* seconds */
    double threshold_time;
    double dilate_time;
    double find_blobs_time;
} ip_context_t;

ip_context_t* ip_create(int stride, int w, int h);

/*
    "ip_threshold8u" replaces all the unsigned bytes in the "input" image > "threshold"
    in the rectangular region of width "w" and height "h" by the value of "set_to" (usually 1 or 255)
    and writes to the "output" image.
    "output" and "input" may point to the exactly same memory location,
    but if they don`t - they should not point into overlapping memory regions.
    "stride" is size in bytes of each row of input and output images.

    Also calculates context.histogram, vertical and horizontal sums context.vs, context.hs
*/

void ip_threshold(ip_context_t* context, void* input, unsigned char threshold, unsigned char set_to, void* output);

/*
    "ip_dilate8unz" replaces all east-west and north-south neighbours in the vicinity of a "radius"
    of each none zero byte in the "input" image in the rectangular region of width "w" and height "h"
    by the value of "set_to" (usually 1 or 255) and writes to the "output" image.
    If "radius" > 1 "manhattan" distance metrics is used to determine pixel neighborhood.
    "output" and "input" may point to the exactly same memory location,
    but if they don`t - they should not point into overlapping memory regions.
    "stride" is size in bytes of each row of both "input" and "output" images.
*/

void ip_dilate(ip_context_t* context, void* input, int radius, unsigned char set_to, void* output);


bool ip_find_blobs(ip_context_t* context, void* input);

void ip_destroy(ip_context_t* context);

void* ip_malloc16(size_t bytes);   /* 16 bytes aligned */
void* ip_malloc16z(size_t bytes);  /* 16 bytes aligned and zero-ed */
void ip_inflate_rect(ip_rect_t* r, int dx, int dy, int min_x, int min_y, int max_x, int max_y);

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_PROCESSING_H */

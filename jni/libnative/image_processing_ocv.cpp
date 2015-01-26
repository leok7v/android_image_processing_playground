#include "image_processing.h"
#include <arm_neon.h>
#pragma push_macro("trace")
#undef trace
#include <opencv2/opencv.hpp>
#pragma pop_macro("trace")


static inline int distance_squared(cv::Point& pt0, cv::Point& pt1) {
    return (pt0.x - pt1.x) * (pt0.x - pt1.x) + (pt0.y - pt1.y) * (pt0.y - pt1.y);
}

static inline bool is_square(std::vector<cv::Point> &si) {
    assert(si.size() == 4);
    return distance_squared(si[0], si[1]) <= 1 && distance_squared(si[1], si[2]) <= 1 &&
           distance_squared(si[2], si[3]) <= 1 && distance_squared(si[3], si[0]) <= 1;
}

static bool ensure_blob_memory(ip_context_t* context, int number_of_blobs) {
//  trace("number_of_blobs=%d", number_of_blobs);
    if (number_of_blobs * (int)sizeof(ip_blob_t) > context->blobs_allocated_bytes) {
        int bytes = number_of_blobs * sizeof(ip_blob_t);
        ip_blob_t* blobs = (ip_blob_t*)ip_malloc16(bytes);
        if (blobs != null) {
            if (context->blobs_allocated_bytes > 0) {
                memcpy(blobs, context->blobs, context->blobs_allocated_bytes);
            }
            free(context->blobs);
            context->blobs_allocated_bytes = bytes;
            context->blobs = blobs;
        } else {
            return false;
        }
    }
    return true;
}

static bool ensure_segments_memory(ip_context_t* context, int number_of_points) {
    if (number_of_points * (int)sizeof(ip_point_t) > context->segments_allocated_bytes) {
        int bytes = number_of_points * sizeof(ip_point_t);
        ip_point_t* segments = (ip_point_t*)ip_malloc16(bytes);
        if (segments != null) {
            if (context->segments_allocated_bytes > 0) {
                memcpy(segments, context->segments, context->segments_allocated_bytes);
            }
            free(context->segments);
            context->segments_allocated_bytes = bytes;
            context->segments = segments;
        } else {
            return false;
        }
    }
    return true;
}

bool ip_find_blobs_in_rect(ip_context_t* context, ip_rect_t nz, std::vector< std::vector<cv::Point> > &contours, void* input) {
    const int w = context->w;
    const int h = context->h;
    assertion(nz.left < nz.right || nz.top < nz.bottom, "expected non empty rectangle %d %d %dx%d",
        nz.left, nz.right, nz.right - nz.left, nz.bottom - nz.top);
//  trace("rectangle %d %d %dx%d", nz.left, nz.right, nz.right - nz.left, nz.bottom - nz.top);
    ip_inflate_rect(&nz, 1, 1, 0, 0, w, h);
//  trace("inflated %d %d %dx%d", nz.left, nz.right, nz.right - nz.left, nz.bottom - nz.top);
    const cv::Rect non_zero(nz.left, nz.top, nz.right - nz.left, nz.bottom - nz.top);
    cv::Mat img(cv::Size(w, h), CV_8UC1, (byte*)input);
    cv::Mat mat(img, non_zero);
    cv::findContours(mat, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0,0));
    const int number_of_blobs = (int)contours.size();
    if (number_of_blobs <= 0) {
        return true;
    }
//  trace("number_of_blobs=%d", number_of_blobs);
    if (!ensure_blob_memory(context, context->number_of_blobs + number_of_blobs)) {
        return false;
    }
    int n = 0;
    for (int i = 0; i < number_of_blobs; i++) { n += (int)contours[i].size(); }
    int number_of_points = n;
    if (!ensure_segments_memory(context, context->number_of_points + number_of_points)) {
        return false;
    }
    int ix = context->number_of_points; // segment memory index
    for (int i = 0; i < number_of_blobs; i++) {
        std::vector<cv::Point> &si = contours[i];
        const int n = (int)si.size();
        ip_blob_t &blob = context->blobs[context->number_of_blobs + i];
        if (n == 1) { // special case - single pixel segment
            const cv::Point &pt = si[0];
            blob.segments_start = -1; // no need to keep segments .center_* defines all
            blob.number_of_segments = 0;
            blob.center_x = nz.left + pt.x;
            blob.center_y = nz.top  + pt.y;
            blob.bounding_rect.left = blob.center_x;
            blob.bounding_rect.top  = blob.center_y;
            blob.bounding_rect.right  = blob.bounding_rect.left + 1;
            blob.bounding_rect.bottom = blob.bounding_rect.top  + 1;
            blob.area = 1;
            blob.arc_length = 1;
            blob.roundness = 0;
        } else if (n == 4 && is_square(si)) { // special case - 2x2 square
            const cv::Point &pt = si[0];
            blob.segments_start = -1;  // no need to keep segments .bounding_rect defines all
            blob.number_of_segments = 0;
            blob.center_x = nz.left + pt.x + 0.5f;
            blob.center_y = nz.top  + pt.y + 0.5f;
            blob.bounding_rect.left = nz.left + pt.x;
            blob.bounding_rect.top  = nz.top  + pt.y;
            blob.bounding_rect.right  = blob.bounding_rect.left + 2;
            blob.bounding_rect.bottom = blob.bounding_rect.top  + 2;
            blob.area = 4;
            blob.arc_length = 4;
            blob.roundness = 0;
        } else {
            blob.segments_start = ix;
            blob.number_of_segments = n;
            for (int j = 0; j < n; j++) {
                const cv::Point &pt = si[j];
                assertion(0 <= pt.x < w && 0 <= pt.y < h, "[%d][%d] x=%d y=%d", i, j, pt.x, pt.y);
//              trace("[%d][%d] x=%d y=%d", i, j, pt.x, pt.y);
                context->segments[ix].x = nz.left + pt.x;
                context->segments[ix].y = nz.top  + pt.y;
                assertion(0 <= context->segments[ix].x < w && 0 <= context->segments[ix].y < h,
                          "[%d][%d] x=%d y=%d", i, j, context->segments[ix].x, context->segments[ix].y);
                ix++;
            }
            const cv::Rect br = cv::boundingRect(si);
            blob.bounding_rect.left = nz.left + br.x;
            blob.bounding_rect.top  = nz.top  + br.y;
            blob.bounding_rect.right  = blob.bounding_rect.left + br.width;
            blob.bounding_rect.bottom = blob.bounding_rect.top  + br.height;
            blob.arc_length = cv::arcLength(si, true); /* true means all non-zeros == 1 */
            const cv::Moments mu = cv::moments(si, true);
            blob.area = mu.m00; /* because openCV returns 0 with 1 pixel narrow blobs */
            if (mu.m00 > 0) {
                blob.center_x = nz.left + mu.m10 / mu.m00;
                blob.center_y = nz.top  + mu.m01 / mu.m00;
            } else {
                blob.center_x = blob.bounding_rect.left + br.width / 2;
                blob.center_y = blob.bounding_rect.top  + br.height / 2;
            }
            // http://gis.stackexchange.com/questions/85812/easily-calculate-roundness-compactness-of-a-polygon
            blob.roundness = blob.arc_length > 0 ? (4.0 * M_PI * blob.area) / (blob.arc_length * blob.arc_length) : 0;
        }
/*
        trace("[%d].contours=%d arc_length=%.3f moments area=%.3f center=%d,%d roundness=%.3f",
               i, n, blob.arc_length, blob.area, (int)blob.center_x, (int)blob.center_y, blob.roundness);
        trace("[%d].contours=%d cv::contourArea()=%d", i, n, (int)cv::contourArea(si));
        trace("[%d].bounding_rect %d,%d %dx%d", i, blob.bounding_rect.left, blob.bounding_rect.top, br.width, br.height);
*/
    }
    context->number_of_blobs += number_of_blobs;
    context->number_of_points += number_of_points;
    // http://en.wikipedia.org/wiki/Image_moment
    // http://docs.opencv.org/trunk/doc/py_tutorials/py_imgproc/py_contours/py_contour_features/py_contour_features.html
    return true;
}

extern "C" bool ip_find_blobs(ip_context_t* context, void* input) {
//  timestamp("ip_find_blobs");
    int64_t start_time = cputime();
    const int w = context->w;
    const int h = context->h;
    context->number_of_blobs = 0;
    context->number_of_points = 0;
    bool b = true;
    if (context->non_zero.left >= context->non_zero.right || context->non_zero.top >= context->non_zero.bottom) {
        b = true;
    } else {
        assertion(context->non_zero_x_ranges * context->non_zero_y_ranges > 0,
                  "is ranges detection broken? non_zero_ranges=%d,%d",
                  context->non_zero_x_ranges, context->non_zero_y_ranges);
        // experimental result 6x6 is still faster than whole area
        if (context->non_zero_x_ranges * context->non_zero_y_ranges <= 36) {
/*
            char label[100];
            sprintf(label, "ip_find_in_nz_blobs %dx%d", context->non_zero_x_ranges, context->non_zero_y_ranges);
            timestamp(label);
*/
            std::vector< std::vector< cv::Point > > contours;
            for (int i = 0; i < context->non_zero_y_ranges && b; i++) {
                ip_rect_t rc;
                rc.top    = context->non_zero_min_y[i];
                rc.bottom = context->non_zero_max_y[i];
                for (int j = 0; j < context->non_zero_x_ranges && b; j++) {
                    rc.left   = context->non_zero_min_x[j];
                    rc.right  = context->non_zero_max_x[j];
                    contours.clear();
                    b = ip_find_blobs_in_rect(context, rc, contours, input);
                }
            }
//          timestamp(label);
        } else {
//          timestamp("ip_find_blobs");
            std::vector< std::vector< cv::Point > > contours;
            contours.reserve(1000);
            b = ip_find_blobs_in_rect(context, context->non_zero, contours, input);
//          timestamp("ip_find_blobs");
        }
    }
    context->find_blobs_time = (cputime() - start_time) / (double)NANOSECONDS_IN_SECOND;
    return b;
}

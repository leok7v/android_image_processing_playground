#include "image_processing.h"
#include <arm_neon.h>
#pragma push_macro("trace")
#undef trace
#include <opencv2/opencv.hpp>
#pragma pop_macro("trace")

extern "C" bool ip_find_blobs(ip_context_t* context, void* input) {
//  timestamp("ip_find_blobs");
    int64_t start_time = cputime();
    const int w = context->w;
    const int h = context->h;
    const ip_rect_t &nz = context->non_zero;
    if (nz.left >= nz.right || nz.top >= nz.bottom) {
        context->number_of_blobs = 0;
        context->number_of_points = 0;
        return true;
    }
    const cv::Rect non_zero(nz.left, nz.top, nz.right - nz.left, nz.bottom - nz.top);
    cv::Mat img(cv::Size(w, h), CV_8UC1, (byte*)input);
    cv::Mat mat(img, non_zero);
    std::vector< std::vector< cv::Point > > contours;
    contours.reserve(200);
    cv::findContours(mat, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0,0));
    int number_of_blobs = (int)contours.size();
//  trace("number_of_blobs=%d", number_of_blobs);
    if (number_of_blobs * (int)sizeof(ip_blob_t) > context->blobs_allocated_bytes) {
        int bytes = number_of_blobs * sizeof(ip_blob_t);
        ip_blob_t* blobs = (ip_blob_t*)malloc(bytes);
        if (blobs != null) {
            free(context->blobs);
            context->blobs_allocated_bytes = bytes;
            context->blobs = blobs;
        } else {
            return false;
        }
    }
    int number_of_points = 0;
    for (int i = 0; i < number_of_blobs; i++) { number_of_points += (int)contours[i].size(); }
    if (number_of_points * (int)sizeof(ip_point_t) > context->segments_allocated_bytes) {
        int bytes = number_of_points * sizeof(ip_point_t);
        ip_point_t* segments = (ip_point_t*)malloc(bytes);
        if (segments != null) {
            free(context->segments);
            context->segments_allocated_bytes = bytes;
            context->segments = segments;
        } else {
            return false;
        }
    }
    context->number_of_blobs = number_of_blobs;
    context->number_of_points = number_of_points;
    int ix = 0; // segment memory index
    for (int i = 0; i < number_of_blobs; i++) {
        std::vector<cv::Point> &si = contours[i];
        const int n = (int)si.size();
        ip_blob_t &blob = context->blobs[i];
        blob.segments_start = ix;
        blob.number_of_segments = n;
        for (int j = 0; j < n; j++) {
            const cv::Point &pt = si[j];
//          trace("[%d][%d] x=%d y=%d", i, j, pt.x, pt.y);
            context->segments[ix].x = nz.left + pt.x;
            context->segments[ix].y = nz.top  + pt.y;
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
/*      ???
        if ((int)blob.area == 0 && (br.width < 2 || br.height < 2)) {
            blob.area = br.width * br.height; // 1 pixel tall or 1 pixel wide bounding rectangle
        } else {
            blob.area += blob.arc_length;
        }
*/
        if (mu.m00 > 0) {
            blob.center_x = nz.left + mu.m10 / mu.m00;
            blob.center_y = nz.top  + mu.m01 / mu.m00;
        } else {
            blob.center_x = blob.bounding_rect.left + br.width / 2;
            blob.center_y = blob.bounding_rect.top  + br.height / 2;
        }
        // http://gis.stackexchange.com/questions/85812/easily-calculate-roundness-compactness-of-a-polygon
        blob.roundness = blob.arc_length > 0 ? (4.0 * M_PI * blob.area) / (blob.arc_length * blob.arc_length) : 0;
/*
        trace("[%d].contours=%d arc_length=%.3f moments area=%.3f center=%d,%d roundness=%.3f",
               i, n, blob.arc_length, blob.area, (int)blob.center_x, (int)blob.center_y, blob.roundness);
        trace("[%d].contours=%d cv::contourArea()=%d", i, n, (int)cv::contourArea(si));
        trace("[%d].bounding_rect %d,%d %dx%d", i, blob.bounding_rect.left, blob.bounding_rect.top, br.width, br.height);
*/
    }
    // http://en.wikipedia.org/wiki/Image_moment
    // http://docs.opencv.org/trunk/doc/py_tutorials/py_imgproc/py_contours/py_contour_features/py_contour_features.html
    context->find_blobs_time = (cputime() - start_time) / (double)NANOSECONDS_IN_SECOND;
//  timestamp("ip_find_blobs");
    return true;
}


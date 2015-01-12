#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H
#include "android.h"

/*
    "ip_threshold8u" replaces all the unsigned bytes in the "input" image > "value"
    in the rectangular region of width "w" and height "h" by the value of "set"
    and writes to the "output" image.
    "output" and "input" may point to the exactly same memory location,
    but if they don`t - they should not point into overlapping memory regions.
    "stride" is size in bytes of each row of input and output images.
*/

void ip_threshold8u(void* input, int stride, int w, int h, unsigned char value, unsigned char set, void* output);

/*
    "ip_dilate8unz" replaces all east-west and north-south neighbours in the vicinity of a "radius"
    of each none zero byte in the "input" image in the rectangular region of width "w" and height "h"
    by the value of "set" and writes to the "output" image.
    If "radius" > 1 "manhattan" distance metrics is used to determine pixel neighborhood.
    "output" and "input" may point to the exactly same memory location,
    but if they don`t - they should not point into overlapping memory regions.
    "stride" is size in bytes of each row of both "input" and "output" images.
*/

void ip_dilate8unz(void* input, int stride, int w, int h, int radius, unsigned char set, void* output);

#endif /* IMAGE_PROCESSING_H */

#include "android.h"
#include <jni.h>
#include "image_processing.h"

void Java_jni_ImageProcessing_threshold8u(JNIEnv* env, jobject that,
                                          jlong input, jint stride, jint w, jint h, jint value,
                                          jint set, jlong output) {
    ip_threshold8u(ll2p(input), stride, w, h, value, set, ll2p(output));
}

void Java_jni_ImageProcessing_dilate8unz(JNIEnv* env, jobject that,
                                         jlong input, jint stride, jint w, jint h, jint radius,
                                         jint set, jlong output) {
    ip_dilate8unz(ll2p(input), stride, w, h, radius, set, ll2p(output));
}

#include "android.h"
#include <jni.h>
#include "image_processing.h"

jlong Java_jni_ImageProcessing_create(JNIEnv* env, jobject that, jint stride, jint w, jint h) {
    return p2ll(ip_create(stride, w, h));
}

static jfieldID numberOfSegments;
static jfieldID segmentsStart;
static jfieldID area;
static jfieldID arcLength;
static jfieldID roundness;
static jfieldID centerX;
static jfieldID centerY;
static jfieldID left;
static jfieldID top;
static jfieldID right;
static jfieldID bottom;

static void bind_blob_fields(JNIEnv* env, jobject blob) {
    jclass cls = (*env)->GetObjectClass(env, blob);
    // http://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/types.html#wp276
    numberOfSegments = (*env)->GetFieldID(env, cls, "numberOfSegments", "I");
    segmentsStart    = (*env)->GetFieldID(env, cls, "segmentsStart", "I");
    area             = (*env)->GetFieldID(env, cls, "area", "F");
    arcLength        = (*env)->GetFieldID(env, cls, "arcLength", "F");
    roundness        = (*env)->GetFieldID(env, cls, "roundness", "F");
    centerX          = (*env)->GetFieldID(env, cls, "centerX", "F");
    centerY          = (*env)->GetFieldID(env, cls, "centerY", "F");
    left             = (*env)->GetFieldID(env, cls, "left", "I");
    top              = (*env)->GetFieldID(env, cls, "top", "I");
    right            = (*env)->GetFieldID(env, cls, "right", "I");
    bottom           = (*env)->GetFieldID(env, cls, "bottom", "I");
    (*env)->DeleteLocalRef(env, cls);
}

void Java_jni_ImageProcessing_threshold(JNIEnv* env, jobject that, jlong context,
                                          jlong input, jint threshold, jint set_to, jlong output) {
    ip_threshold(ll2p(context), ll2p(input), threshold, set_to, ll2p(output));
}

void Java_jni_ImageProcessing_dilate(JNIEnv* env, jobject that, jlong context,
                                         jlong input, jint radius, jint set_to, jlong output) {
    ip_dilate(ll2p(context), ll2p(input), radius, set_to, ll2p(output));
}

jboolean Java_jni_ImageProcessing_findBlobs(JNIEnv* env, jobject that, jlong context, jlong input) {
    return ip_find_blobs(ll2p(context), ll2p(input));
}

jint Java_jni_ImageProcessing_numberOfBlobs(JNIEnv* env, jobject that, jlong context) {
    ip_context_t* ctx = (ip_context_t*)ll2p(context);
    return ctx->number_of_blobs;
}

void Java_jni_ImageProcessing_getBlob(JNIEnv* env, jobject that, jlong context, jint i, jobject blob) {
    ip_context_t* ctx = (ip_context_t*)ll2p(context);
    if (numberOfSegments == null) {
        bind_blob_fields(env, blob);
    }
    (*env)->SetIntField(env, blob, numberOfSegments, ctx->blobs[i].number_of_segments);
    (*env)->SetIntField(env, blob, segmentsStart, ctx->blobs[i].segments_start);

    (*env)->SetFloatField(env, blob, area     , ctx->blobs[i].area);
    (*env)->SetFloatField(env, blob, arcLength, ctx->blobs[i].arc_length);
    (*env)->SetFloatField(env, blob, roundness, ctx->blobs[i].roundness);
    (*env)->SetFloatField(env, blob, centerX  , ctx->blobs[i].center_x);
    (*env)->SetFloatField(env, blob, centerY  , ctx->blobs[i].center_y);

    (*env)->SetIntField(env, blob, left  , ctx->blobs[i].bounding_rect.left);
    (*env)->SetIntField(env, blob, top   , ctx->blobs[i].bounding_rect.top);
    (*env)->SetIntField(env, blob, right , ctx->blobs[i].bounding_rect.right);
    (*env)->SetIntField(env, blob, bottom, ctx->blobs[i].bounding_rect.bottom);
}

jint Java_jni_ImageProcessing_numberOfPoints(JNIEnv* env, jobject that, jlong context) {
    ip_context_t* ctx = (ip_context_t*)ll2p(context);
    return ctx->number_of_points;
}

jlong Java_jni_ImageProcessing_segments(JNIEnv* env, jobject that, jlong context) {
    ip_context_t* ctx = (ip_context_t*)ll2p(context);
    return p2ll(ctx->segments);
}

jdouble Java_jni_ImageProcessing_thresholdTime(JNIEnv* env, jobject that, jlong context) {
    ip_context_t* ctx = (ip_context_t*)ll2p(context);
    return ctx->threshold_time;
}

jdouble Java_jni_ImageProcessing_dilateTime(JNIEnv* env, jobject that, jlong context) {
    ip_context_t* ctx = (ip_context_t*)ll2p(context);
    return ctx->dilate_time;
}

jdouble Java_jni_ImageProcessing_findBlobsTime(JNIEnv* env, jobject that, jlong context) {
    ip_context_t* ctx = (ip_context_t*)ll2p(context);
    return ctx->find_blobs_time;
}

void Java_jni_ImageProcessing_destroy(JNIEnv* env, jobject that, jlong context) {
    ip_destroy(ll2p(context));
}

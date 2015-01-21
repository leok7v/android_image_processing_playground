#include "android.h"
#include <jni.h>

jint Java_jni_Native_strlen(JNIEnv* env, jobject that, jlong address) {
    return strlen((const char*)ll2p(address));
}

void Java_jni_Native_memcpyAB(JNIEnv* env, jobject that, jlong address, jbyteArray data, jint offset, jint length) {
    jbyte* a = (*env)->GetByteArrayElements(env, data, NULL);
    jsize  bytes = (*env)->GetArrayLength(env, data);
//  trace("%p %p offset=%d length=%d bytes=%d", ll2p(address), a, offset, length, bytes);
    assertion(0 <= offset && offset + length <= bytes, "offset=%d length=%d bytes=%d", offset, length, bytes);
    memcpy(ll2p(address), a + offset, length);
    (*env)->ReleaseByteArrayElements(env, data, a, 0);
}

void Java_jni_Native_memcpyBA(JNIEnv* env, jobject that, jbyteArray data, jlong address, jint offset, jint length) {
    jbyte* a = (*env)->GetByteArrayElements(env, data, NULL);
    jsize  bytes = (*env)->GetArrayLength(env, data);
//  trace("%p %p offset=%d length=%d bytes=%d", ll2p(address), a, offset, length, bytes);
    assertion(0 <= offset && offset + length <= bytes, "offset=%d length=%d bytes=%d", offset, length, bytes);
    memcpy(a + offset, ll2p(address), length);
    (*env)->ReleaseByteArrayElements(env, data, a, 0);
}

void Java_jni_Native_memcpyAI(JNIEnv* env, jobject that, jlong address, jintArray data, jint offset, jint length) {
    jsize bytes = (*env)->GetArrayLength(env, data) * sizeof(jint);
    jint* ia = (*env)->GetIntArrayElements(env, data, NULL);
    jbyte* a = (jbyte*)ia;
    offset *= sizeof(jint);
    length *= sizeof(jint);
//  trace("%p %p offset=%d length=%d bytes=%d", ll2p(address), a, offset, length, bytes);
    assertion(0 <= offset && offset + length <= bytes, "offset=%d length=%d bytes=%d", offset, length, bytes);
    memcpy(ll2p(address), a + offset, length);
    (*env)->ReleaseIntArrayElements(env, data, ia, 0);
}

void Java_jni_Native_memcpyIA(JNIEnv* env, jobject that, jintArray data, jlong address, jint offset, jint length) {
    jsize  bytes = (*env)->GetArrayLength(env, data) * sizeof(jint);
    jint* ia = (*env)->GetIntArrayElements(env, data, NULL);
    jbyte* a = (jbyte*)ia;
    offset *= sizeof(jint);
    length *= sizeof(jint);
//  trace("%p %p offset=%d length=%d bytes=%d", ll2p(address), a, offset, length, bytes);
    assertion(0 <= offset && offset + length <= bytes, "offset=%d length=%d bytes=%d", offset, length, bytes);
    memcpy(a + offset, ll2p(address), length);
    (*env)->ReleaseIntArrayElements(env, data, ia, 0);
}

void Java_jni_Native_memcpyAF(JNIEnv* env, jobject that, jlong address, jfloatArray data, jint offset, jint length) {
    jsize bytes = (*env)->GetArrayLength(env, data) * sizeof(jint);
    jfloat* fa = (*env)->GetFloatArrayElements(env, data, NULL);
    jbyte*   a = (jbyte*)fa;
    offset *= sizeof(jint);
    length *= sizeof(jint);
//  trace("%p %p offset=%d length=%d bytes=%d", ll2p(address), a, offset, length, bytes);
    assertion(0 <= offset && offset + length <= bytes, "offset=%d length=%d bytes=%d", offset, length, bytes);
    memcpy(ll2p(address), a + offset, length);
    (*env)->ReleaseFloatArrayElements(env, data, fa, 0);
}

void Java_jni_Native_memcpyFA(JNIEnv* env, jobject that, jfloatArray data, jlong address, jint offset, jint length) {
    jsize bytes = (*env)->GetArrayLength(env, data) * sizeof(jint);
    jfloat* fa = (*env)->GetFloatArrayElements(env, data, NULL);
    jbyte*   a = (jbyte*)fa;
    offset *= sizeof(jfloat);
    length *= sizeof(jfloat);
//  trace("%p %p offset=%d length=%d bytes=%d", ll2p(address), a, offset, length, bytes);
    assertion(0 <= offset && offset + length <= bytes, "offset=%d length=%d bytes=%d", offset, length, bytes);
    memcpy(a + offset, ll2p(address), length);
    (*env)->ReleaseFloatArrayElements(env, data, fa, 0);
}

void Java_jni_Native_memset(JNIEnv* env, jobject that, jlong address, jint data, jint bytes) {
    memset(ll2p(address), data, bytes);
}

jlong Java_jni_Native_malloc(JNIEnv* env, jobject that, jint bytes) {
    return p2ll(mem_allocz(bytes));
}

void Java_jni_Native_free(JNIEnv* env, jobject that, jlong address) {
    mem_free(ll2p(address));
}

void Java_jni_Native_memcpy(JNIEnv* env, jobject that, jlong to, jlong from, jint bytes) {
    memcpy(ll2p(to), ll2p(from), bytes);
}

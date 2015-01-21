#include "android.h"
#include <jni.h>
#include <android/bitmap.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

void Java_jni_Android_setThreadAffinityNextCore(JNIEnv* env, jobject that) {
    static volatile int core_id;
    setaffinity_and_inc(&core_id);
}

jint Java_jni_Android_getgid(JNIEnv* env, jobject that) {
    return getgid();
}

jint Java_jni_Android_getuid(JNIEnv* env, jobject that) {
    return getuid();
}

jint Java_jni_Android_setgid(JNIEnv* env, jobject that, jint gid) {
    if (setgid(gid) != 0) {
        return -errno;
    }
    return 0;
}

jint Java_jni_Android_setuid(JNIEnv* env, jobject that, jint uid) {
    if (setuid(uid) != 0) {
        return -errno;
    }
    return 0;
}


/* TODO: updateBitmap etc deserves it's own library */

static void grayscaleToRGBA(unsigned int* pixels, AndroidBitmapInfo* info, jlong bits, jint width, jint height, jboolean mirror) {
    register int w = info->width;
    register int h = info->height;
    register int stride = info->stride / 4; // in pixels
//  trace("%x%d stride=%d", w, h, stride);
    register unsigned int* pix = pixels;
    // ~time: 500-2500 microseconds for 640x480 grayscale bitmap
    if (mirror) {
        if (w == width && h == height) {
            register unsigned char* a = ll2p(bits);
            for (register int y = 0; y < height; y++) {
                register unsigned int* b = pix + width;
                register unsigned char* e = a + width;
                while (a != e) {
                    register unsigned int c = *a++;
                    *(--b) = 0xFF000000U | (0x010101U * c);
                }
                pix += stride;
            }
        } else if (w == width / 2 && h == height / 2) {
            register unsigned short* a0 = ll2p(bits);
            register unsigned short* a1 = a0 + w;
            for (register int y = 0; y < h; y++) {
                register unsigned int* b = pix + w;
                register unsigned short* e = a1;
                while (a0 != e) {
                    register int c = ((*a0 >> 8) + (*a0 & 0xFF) +
                                      (*a1 >> 8) + (*a1 & 0xFF) + 4) / 4 - 1;
                    *(--b) = 0xFF000000U | (0x010101U * c);
                    a0++;
                    a1++;
                }
                a0 = a1;
                a1 += w;
                pix += stride;
            }
        } else {
            assertion(false, "only half size scale is supported w=%d h=%d bitmap %dx%d",
                              w, h, width, height);
        }
    } else if (w == width && h == height) {
        register unsigned char* a = ll2p(bits);
        for (register int y = 0; y < height; y++) {
            register unsigned int* b = pix;
            register unsigned char* e = a + width;
            while (a != e) {
                register unsigned int c = *a++;
                *b++ = 0xFF000000U | (0x010101U * c);
            }
            pix += stride;
        }
    } else if (w == width / 2 && h == height / 2) {
        register unsigned short* a0 = ll2p(bits);
        register unsigned short* a1 = a0 + w;
        for (register int y = 0; y < h; y++) {
            register unsigned int* b = pix;
            register unsigned short* e = a1;
            while (a0 != e) {
                register int c = ((*a0 >> 8) + (*a0 & 0xFF) +
                                  (*a1 >> 8) + (*a1 & 0xFF) + 4) / 4 - 1;
                *b++ = 0xFF000000U | (0x010101U * c);
                a0++;
                a1++;
            }
            a0 = a1;
            a1 += w;
            pix += stride;
        }
    } else {
        assertion(false, "only half size scale is supported w=%d h=%d bitmap %dx%d",
                          w, h, width, height);
    }
}


void Java_jni_Android_updateBitmap(JNIEnv* env, jobject that, jobject bitmap, jlong data, jint w, jint h, jint bpp, jboolean mirror) {
    // timestamp: 122..305 microseconds mean=152. Contested lock up to 5800 mcs
//  timestamp("updateBitmap");
    AndroidBitmapInfo info = {0};
    int r = AndroidBitmap_getInfo(env, bitmap, &info);
    if (r != 0) {
        trace("AndroidBitmap_getInfo() failed ! error=%d", r);
        return;
    }
    int width = info.width;
    int height = info.height;
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888 && info.format != ANDROID_BITMAP_FORMAT_A_8) {
        trace("Bitmap format is not RGBA_8888 or A_8");
        return;
    }
    int bytesPerPixel = info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ? 4 : 1;
    void* pixels = null;
    r = AndroidBitmap_lockPixels(env, bitmap, &pixels);
    if (r != 0) {
        trace("AndroidBitmap_lockPixels() failed ! error=%d", r);
        return;
    }
    if (w == width && h == height && bytesPerPixel == bpp) {
        assertion(!mirror, "mirror is not supported yet");
        memcpy(pixels, ll2p(data), width * height * bytesPerPixel);
    } else if (bytesPerPixel == 4 && bpp == 1) {
        grayscaleToRGBA(pixels, &info, data, w, h, mirror);
    } else {
        assertion(bytesPerPixel == 4 && bpp == 1, "only grayscale -> RGBA is supported bytesPerPixel=%d bpp=%d", bytesPerPixel, bpp);
    }
    AndroidBitmap_unlockPixels(env, bitmap);
//  timestamp("updateBitmap");
}


static void* deliberately_leaked_to_check_if_detection_is_working;

/* static */ void* deliberately_leak_to_check_if_detection_is_working() { // non-static to see on stack trace
    deliberately_leaked_to_check_if_detection_is_working = mem_allocz(153);
    return deliberately_leaked_to_check_if_detection_is_working;
}

void Java_jni_Android_clearMemoryLeaks(JNIEnv* env, jobject that) {
    mem_clear_leaks();
}

void Java_jni_Android_checkMemoryLeaks(JNIEnv* env, jobject that) {
    void* leak = deliberately_leak_to_check_if_detection_is_working();
    mem_dump_leaks();
    mem_free(leak);
}

void Java_jni_Android_traceAllocs(JNIEnv* env, jobject that, jboolean on) {
    mem_trace_allocs(on);
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    mem_clear_leaks();
    deliberately_leak_to_check_if_detection_is_working();
    return JNI_VERSION_1_6;
}

void JNI_OnUnload(JavaVM *vm, void *reserved) {
    mem_dump_leaks();
}

#include "libyuv.h"

#ifdef __ANDROID__
#include "android.h"
#include <jni.h>

extern "C" {

// see: http://msdn.microsoft.com/en-us/library/ms867704.aspx#yuvformats_420formats_12bitsperpixel

jboolean Java_jni_libyuv_NV21toABGR(JNIEnv* env, jobject that, jbyteArray nv21, jlong argb, jint w, jint h) {
    jbyte* yvu = env->GetByteArrayElements(nv21, NULL);
    uint8* dst_argb = (uint8*)ll2p(argb);
    int src_stride_y = w;
    const uint8* src_vu = (const uint8*)yvu + w * h;
    const uint8* src_y = (const uint8*)yvu;
    int src_stride_vu = w;
    int dst_stride_argb = w * sizeof(int);
    int r = libyuv::NV21ToARGB(src_y, src_stride_y, src_vu, src_stride_vu, dst_argb, dst_stride_argb, w, h);
    r = libyuv::ARGBToABGR(dst_argb, dst_stride_argb, dst_argb, dst_stride_argb, w, h);
    env->ReleaseByteArrayElements(nv21, yvu, 0);
    return r == 0;
}

} /* extern "C" */

#endif

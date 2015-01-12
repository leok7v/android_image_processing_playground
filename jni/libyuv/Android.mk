LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

APP_MODULES += libyuv
LOCAL_MODULE := libyuv
APP_PLATFORM := android-17
LOCAL_CFLAGS += -DLIBYUV_NEON -mfpu=neon
LOCAL_CFLAGS += -Wall -fexceptions

LOCAL_C_INCLUDES += $(LOCAL_PATH)/files/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc

LOCAL_SRC_FILES := \
    files/source/compare_neon.cc \
    files/source/rotate_neon.cc \
    files/source/row_neon.cc \
    files/source/scale_neon.cc \
    files/source/compare.cc \
    files/source/convert.cc \
    files/source/convert_argb.cc \
    files/source/convert_from.cc \
    files/source/cpu_id.cc \
    files/source/format_conversion.cc \
    files/source/planar_functions.cc \
    files/source/rotate.cc \
    files/source/rotate_argb.cc \
    files/source/row_common.cc \
    files/source/row_posix.cc \
    files/source/scale.cc \
    files/source/scale_argb.cc \
    files/source/video_common.cc

LOCAL_SRC_FILES += NV21toABGR.cc

LOCAL_CPP_EXTENSION := .cc

LOCAL_SDK_VERSION := 17
LOCAL_NDK_STL_VARIANT := stlport_shared

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)


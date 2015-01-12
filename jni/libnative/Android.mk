LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := native
APP_MODULES  += native

LOCAL_CFLAGS += -mfpu=neon

ifeq ($(APP_OPTIM),release)
  LOCAL_CFLAGS += -O3
  $(info "LOCAL_CFLAGS=$(LOCAL_CFLAGS)")
endif

#see: https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
AGGRESSIVE := -faggressive-loop-optimizations -funroll-all-loops --param max-unrolled-insns=32 --param max-unroll-times=32
AGGRESSIVE += -fvariable-expansion-in-unroller -fprefetch-loop-arrays -frename-registers
AGGRESSIVE += -fmodulo-sched -fmodulo-sched-allow-regmoves
AGGRESSIVE += -finline-small-functions -finline-functions -finline-functions-called-once -finline-limit=1024
#LOCAL_CFLAGS += $(AGGRESSIVE) # has negative effect when -O3 is on

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc
LOCAL_SRC_FILES := android.c mem.c mapsl.c mapll.c timestamp.c trace.c backtrace.c
LOCAL_SRC_FILES += image_processing.c
LOCAL_SRC_FILES += image_processing_ocv.cpp
LOCAL_SRC_FILES += jni_native.c jni_android.c jni_image_processing.c

LOCAL_CPPFLAGS  += -std=c++0x
LOCAL_C99_FILES := $(filter %.c, $(LOCAL_SRC_FILES))
TARGET-process-src-files-tags += $(call add-src-files-target-cflags, $(LOCAL_C99_FILES), -std=c99)

LOCAL_LDLIBS    += -llog -ljnigraphics -lGLESv1_CM -landroid
LOCAL_STATIC_LIBRARIES := cpufeatures

include $(BUILD_SHARED_LIBRARY)



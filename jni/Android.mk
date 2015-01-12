LOCAL_PATH := $(call my-dir)
ROOT_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_ARM_NEON := true

include $(ROOT_LOCAL_PATH)/prebuild/Android.mk
include $(ROOT_LOCAL_PATH)/libnative/Android.mk
include $(ROOT_LOCAL_PATH)/libyuv/Android.mk

APP_CFLAGS += -Wno-unused-variable
LOCAL_CFLAGS += -Wno-unused-variable
$(call import-module,android/cpufeatures)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE = opencv_java
LOCAL_MODULE_FILENAME = libopencv_java
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libopencv_java.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE = opencv_info
LOCAL_MODULE_FILENAME = libopencv_info
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libopencv_info.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE = native_camera_r4.2.0
LOCAL_MODULE_FILENAME = libnative_camera_r4.2.0
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libnative_camera_r4.2.0.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE = native_camera_r4.3.0
LOCAL_MODULE_FILENAME = libnative_camera_r4.3.0
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libnative_camera_r4.3.0.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE = native_camera_r4.4.0
LOCAL_MODULE_FILENAME = libnative_camera_r4.4.0
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libnative_camera_r4.4.0.so
include $(PREBUILT_SHARED_LIBRARY)

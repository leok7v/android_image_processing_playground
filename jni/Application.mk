
# cannot do armeabi-v7a-hard till I rebuild opencv with neon and armeabi-v7a-hard
#APP_ABI := armeabi-v7a-hard
APP_ABI := armeabi-v7a
APP_PLATFORM := android-17
APP_STL := gnustl_shared
CPU=armv7-a
OPTIMIZE_CFLAGS = "-mfloat-abi=mhard-float -mfpu=neon -marm -march=$CPU "
APP_CFLAGS = -Wall -funwind-tables $(ANT_CFLAGS) -fexceptions -Wno-unused-variable
APP_CPPFLAGS = -std=c++11

#for: armeabi-v7a-hard
#TARGET_CFLAGS += -mhard-float -D_NDK_MATH_NO_SOFTFP=1
#TARGET_LDFLAGS += -Wl,--no-warn-mismatch -lm_hard

ifeq ("$(NDK_DEBUG)","1")
    $(info "NDK_DEBUG=1")
    APP_CFLAGS += -DDEBUG -UNDEBUG -g
    APP_OPTIM := debug
else
    $(info "NDK_DEBUG=0")
    APP_CFLAGS += -UDEBUG -DNDEBUG -DRELEASE -Ofast
    APP_OPTIM := release
endif
$(info "APP_OPTIM=$(APP_OPTIM)")

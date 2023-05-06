LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
	$(LOCAL_PATH)/../SDL2_image/ \
	$(LOCAL_PATH)/../SDL2_mixer/ \
	$(LOCAL_PATH)/../SDL2_ttf/   \
	$(LOCAL_PATH)/../Box2D

# Add your application source files here...
LOCAL_SRC_FILES :=  \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/*.cpp) )

LOCAL_CFLAGS := -D_ANDROID

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_mixer SDL2_ttf Box2D

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid

include $(BUILD_SHARED_LIBRARY)

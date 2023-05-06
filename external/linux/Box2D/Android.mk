LOCAL_PATH := $(call my-dir)

###########################
#
# Box2D shared library
#
###########################

include $(CLEAR_VARS)

LOCAL_MODULE := Box2D

LOCAL_C_INCLUDES := \
$(LOCAL_PATH)/Box2D \
$(LOCAL_PATH)/Box2D/Common \
$(LOCAL_PATH)/Box2D/Collision \
$(LOCAL_PATH)/Box2D/Collision/Shapes \
$(LOCAL_PATH)/Box2D/Dynamics \
$(LOCAL_PATH)/Box2D/Dynamics/Contacts \
$(LOCAL_PATH)/Box2D/Joints

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/Box2D/Common/*.cpp) \
	$(wildcard $(LOCAL_PATH)/Box2D/Rope/*.cpp) \
	$(wildcard $(LOCAL_PATH)/Box2D/Dynamics/*.cpp) \
	$(wildcard $(LOCAL_PATH)/Box2D/Dynamics/Joints/*.cpp) \
	$(wildcard $(LOCAL_PATH)/Box2D/Dynamics/Contacts/*.cpp) \
	$(wildcard $(LOCAL_PATH)/Box2D/Collision/*.cpp) \
	$(wildcard $(LOCAL_PATH)/Box2D/Collision/Shapes/*.cpp) )

# Warnings we haven't fixed (yet)
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-sign-compare

LOCAL_LDLIBS := -ldl -llog -landroid

ifeq ($(NDK_DEBUG),1)
    cmd-strip :=
endif

include $(BUILD_SHARED_LIBRARY)

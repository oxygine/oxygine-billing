LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := oxygine-billing_static
LOCAL_MODULE_FILENAME := liboxygine-billing

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../oxygine-framework/oxygine/src/ \
					$(LOCAL_PATH)/src/ \


LOCAL_SRC_FILES :=  src/billing.cpp \
					src/android/AndroidBilling.cpp \

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/src/

include $(BUILD_STATIC_LIBRARY)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    vvnx.cpp \
    action.cpp 


LOCAL_SHARED_LIBRARIES := \
    libbinder \
    libutils \
    libcutils \
    libbase \
    libsqlite \
    android.hardware.gnss@1.0 \
    libhidlbase



LOCAL_MODULE:= gpsvvnx

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += -Wall -Werror -Wunused -Wunreachable-code

include $(BUILD_EXECUTABLE)

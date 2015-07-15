LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := dtn2-bytewalla-app
LOCAL_SRC_FILES := queue.c file_watch.c file_queue.c
#LOCAL_CFLAGS += -I/home/wwtao/Desktop/bluetooth/include/include
#LOCAL_LDLIBS += -lpthread
include $(BUILD_EXECUTABLE)
#include $(BUILD_STATIC_LIBRARY)
#include $(BUILD_SHARED_LIBRARY)

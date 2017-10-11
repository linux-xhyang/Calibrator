LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS:= optional
LOCAL_C_INCLUDES := bionic

LOCAL_SRC_FILES:= cache-detector.cc

LOCAL_MODULE:= cache-detector
LOCAL_CPP_EXTENSION := .cc

LOCAL_SHARED_LIBRARIES :=   \
    libc

LOCAL_STATIC_LIBRARIES:=

include $(BUILD_EXECUTABLE)


ifeq ($(HOST_OS),linux)
include $(CLEAR_VARS)
LOCAL_MODULE := cache-detector
LOCAL_SRC_FILES:= cache-detector.cc
LOCAL_LDLIBS += -lpthread
LOCAL_CPP_EXTENSION := .cc
include $(BUILD_HOST_EXECUTABLE)
endif

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS:= optional
LOCAL_C_INCLUDES := bionic

LOCAL_SRC_FILES:= calibrator.cc
LOCAL_CPP_EXTENSION := .cc

LOCAL_MODULE:= calibrator

LOCAL_SHARED_LIBRARIES :=   \
    libc

LOCAL_STATIC_LIBRARIES:=

include $(BUILD_EXECUTABLE)


ifeq ($(HOST_OS),linux)
include $(CLEAR_VARS)
LOCAL_MODULE := calibrator
LOCAL_SRC_FILES:= calibrator.cc
LOCAL_LDLIBS += -lpthread
LOCAL_CPP_EXTENSION := .cc
include $(BUILD_HOST_EXECUTABLE)
endif

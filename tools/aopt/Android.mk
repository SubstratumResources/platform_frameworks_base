#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH:= $(call my-dir)

aoptMain := Main.cpp
aoptSources := \
    AoptAssets.cpp \
    AoptConfig.cpp \
    AoptUtil.cpp \
    AoptXml.cpp \
    ApkBuilder.cpp \
    Command.cpp \
    CrunchCache.cpp \
    FileFinder.cpp \
    Images.cpp \
    Package.cpp \
    pseudolocalize.cpp \
    Resource.cpp \
    ResourceFilter.cpp \
    ResourceIdCache.cpp \
    ResourceTable.cpp \
    SourcePos.cpp \
    StringPool.cpp \
    WorkQueue.cpp \
    XMLNode.cpp \
    ZipEntry.cpp \
    ZipFile.cpp \
    logstubs.cpp 

aoptHostStaticLibs := \
    libandroidfw-static \
    libpng \
    liblog \
    libexpat_static \
    libutils \
    libcutils \
    libziparchive \
    libbase \
    libm \
    libc \
    libz \
    libc++_static 

CFLAGS := \
	-DHAVE_SYS_UIO_H \
	-DHAVE_PTHREADS \
	-DHAVE_SCHED_H \
	-DHAVE_SYS_UIO_H \
	-DHAVE_IOCTL \
	-DHAVE_TM_GMTOFF \
	-DANDROID_SMP=1  \
	-DHAVE_ENDIAN_H \
	-DHAVE_POSIX_FILEMAP \
	-DHAVE_OFF64_T \
	-DHAVE_ENDIAN_H \
	-DHAVE_SCHED_H \
	-DHAVE_LITTLE_ENDIAN_H \
	-D__ANDROID__ \
	-D_ANDROID_CONFIG_H \
	-D_BYPASS_DSO_ERROR 
	
CFLAGS += -Wno-format-y2k
CFLAGS += -DSTATIC_ANDROIDFW_FOR_TOOLS
CFLAGS += -DHAVE_ANDROID_OS=1
CFLAGS += -D'AOPT_VERSION="$(PLATFORM_VERSION)-$(TARGET_BUILD_VARIANT)"' 
CFLAGS += -Wall -Werror -Wunreachable-code -Wno-error=unused-but-set-variable

aoptCppFlags := -std=gnu++11 -Wno-missing-field-initializers -g -O3 
aoptHostLdLibs := -lc -lgcc -ldl -lz -lm -lstdc++
aoptIncludes := \
        $(LOCAL_PATH)/include \
        system/core/base/include \
        system/core/libutils \
        system/core/liblog \
        system/core/libcutils \
        $(LOCAL_PATH)/libpng \
        external/expat \
        external/zlib \
        bionic/libc/include \
        external/libcxx/include \
        system/core/libziparchive \
	$(LOCAL_PATH)/libaoptfw 
   
include $(CLEAR_VARS)

ANDROIDFW_PATH := ../../libs/androidfw
LOCAL_C_INCLUDES := $(LOCAL_PATH)/
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

androidfw_srcs := \
    $(ANDROIDFW_PATH)/Asset.cpp \
    $(ANDROIDFW_PATH)/AssetDir.cpp \
    $(ANDROIDFW_PATH)/AssetManager.cpp \
    $(ANDROIDFW_PATH)/misc.cpp \
    $(ANDROIDFW_PATH)/ObbFile.cpp \
    $(ANDROIDFW_PATH)/ResourceTypes.cpp \
    $(ANDROIDFW_PATH)/StreamingZipInflater.cpp \
    $(ANDROIDFW_PATH)/TypeWrappers.cpp \
    $(ANDROIDFW_PATH)/ZipFileRO.cpp \
    $(ANDROIDFW_PATH)/ZipUtils.cpp \
    $(ANDROIDFW_PATH)/BackupData.cpp \
    $(ANDROIDFW_PATH)/BackupHelpers.cpp \
    $(ANDROIDFW_PATH)/CursorWindow.cpp

LOCAL_MODULE:= libandroidfw-static
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -DSTATIC_ANDROIDFW_FOR_TOOLS
LOCAL_CFLAGS += -Wall -Werror -Wunused -Wunreachable-code
LOCAL_C_INCLUDES := external/zlib
LOCAL_STATIC_LIBRARIES := libziparchive libbase
ifdef TARGET_2ND_ARCH
LOCAL_MULTILIB := both
LOCAL_SRC_FILES_64 := $(androidfw_srcs)
LOCAL_SRC_FILES_32 := $(androidfw_srcs)
else
LOCAL_SRC_FILES := $(androidfw_srcs)
endif
include $(BUILD_STATIC_LIBRARY)

# Build the host static library: libaopt
# ==========================================================
include $(CLEAR_VARS)

LOCAL_MODULE := libaopt
LOCAL_CFLAGS := -DSTATIC_ANDROIDFW_FOR_TOOLS $(aoptCFlags) $(CFLAGS) 
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/../
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include 
LOCAL_CPPFLAGS := $(aoptCppFlags)
LOCAL_C_INCLUDES := $(aoptIncludes)

LOCAL_MODULE_STEM_32 := libaopt

ifdef TARGET_2ND_ARCH
LOCAL_MULTILIB := both
LOCAL_SRC_FILES_64 := $(aoptSources) 
LOCAL_SRC_FILES_32 := $$(aoptSources) 
else
LOCAL_SRC_FILES := $(aoptSources) 
endif
LOCAL_STATIC_LIBRARIES := $(aoptHostStaticLibs)
include $(BUILD_STATIC_LIBRARY)

# ==========================================================
# Build the host executable: aopt
# ==========================================================
include $(CLEAR_VARS)

LOCAL_MODULE := aopt
LOCAL_CFLAGS := -Wno-format-y2k -DSTATIC_ANDROIDFW_FOR_TOOLS $(aoptCFlags) $(CFLAGS) 
LOCAL_CPPFLAGS := $(aoptCppFlags) -D__ANDROID__
LOCAL_C_INCLUDES := $(aoptIncludes)
LOCAL_STATIC_LIBRARIES := $(aoptHostStaticLibs)        
LOCAL_LDLIBS := $(aoptHostLdLibs) 
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_STEM_32 := aopt        
LOCAL_MODULE_PATH_64 := $(ANDROID_PRODUCT_OUT)/system/bin

ifdef TARGET_2ND_ARCH
LOCAL_MULTILIB := both
LOCAL_SRC_FILES_64 := $(aoptMain) $(aoptSources)
LOCAL_SRC_FILES_32 := $(aoptMain) $(aoptSources)
else
LOCAL_SRC_FILES := $(aoptMain) $(aoptSources)
endif

LOCAL_MODULE_CLASS := UTILITY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_UNSTRIPPED_PATH := $(PRODUCT_OUT)/symbols/utilities
LOCAL_LDFLAGS += \
-static -Wl,-rpath-link=/data/toolchain/sysroot/usr/lib/ \
-dynamic-linker,/system/bin/linker 
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_PACK_MODULE_RELOCATIONS := false
include $(BUILD_EXECUTABLE)

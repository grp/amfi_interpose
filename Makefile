include theos/makefiles/common.mk

export TARGET_LD=xcrun -sdk iphoneos clang
export GO_EASY_ON_ME=1

ifeq ($(PLATFORM),swift)
	export ARCHS=armv7s
else
	export ARCHS=armv7
endif

override TARGET_CODESIGN = xcrun -sdk iphoneos codesign
override TARGET_CODESIGN_FLAGS = -fs "iPhone Developer"

LIBRARY_NAME = amfi_interpose
amfi_interpose_FILES = amfi_interpose.c
amfi_interpose_FRAMEWORKS = CoreFoundation
amfi_interpose_LDFLAGS = -lmis

TOOL_NAME = jailbreak
jailbreak_FILES = jailbreak.m
jailbreak_FRAMEWORKS = CoreFoundation
jailbreak_PRIVATE_FRAMEWORKS = MobileCoreServices

include $(THEOS_MAKE_PATH)/library.mk
include $(THEOS_MAKE_PATH)/tool.mk

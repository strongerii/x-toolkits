##
## /S2l/anviz/log/make.inc
##
## History:
##   2015/01/26 - [ypchang] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

ifeq ($(BUILD_ANVIZ_APPLICATION_JSONCPP), y)
LOCAL_PATH := $(call my-dir)

### libjsoncpp.so
include $(CLEAR_VARS)

THIS_SVR_TARGET   = libjsoncpp.so
THIS_SVR_VER_FILE = $(ANVIZ_DIR)/jsoncpp/json/version.h
THIS_SVR_PREFIX   = JSONCPP

LOCAL_TARGET := $(THIS_SVR_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                             \
                -std=c++11 -Werror

LOCAL_LIBS   := 

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/sqlite/usr/lib -lsqlite3 \
                 -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

THIS_SVR_SO_VER_MAJ := $(shell awk '/define\ $(THIS_SVR_PREFIX)_LIB_MAJOR/{print $$3}' $(THIS_SVR_VER_FILE))
THIS_SVR_SO_VER_MIN := $(shell awk '/define\ $(THIS_SVR_PREFIX)_LIB_MINOR/{print $$3}' $(THIS_SVR_VER_FILE))
THIS_SVR_SO_VER_PAT := $(shell awk '/define\ $(THIS_SVR_PREFIX)_LIB_PATCH/{print $$3}' $(THIS_SVR_VER_FILE))
THIS_SVR_SO_VER_STR := $(THIS_SVR_SO_VER_MAJ).$(THIS_SVR_SO_VER_MIN).$(THIS_SVR_SO_VER_PAT)

THIS_SVR_SO_NAME   := $(patsubst %.so,%.so.$(THIS_SVR_SO_VER_MAJ), $(THIS_SVR_TARGET))
THIS_SVR_SO_TARGET := $(patsubst %.so,%.so.$(THIS_SVR_SO_VER_STR), $(THIS_SVR_TARGET))

LOCAL_SO_NAME := $(THIS_SVR_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ANVIZ_LIB_DIR)
	@cp -dpRf $< $(ANVIZ_LIB_DIR)/$(THIS_SVR_SO_TARGET)
	@ln -sf $(THIS_SVR_SO_TARGET) $(ANVIZ_LIB_DIR)/$(THIS_SVR_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

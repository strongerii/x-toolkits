##########################################################################################
#	prepare param
#########################################################################################

ifeq ($(PARAM_FILE), )
    PARAM_FILE:= ../../HisiSDK/mpp/Makefile.param
    include $(PARAM_FILE)
endif

TARGET_DIR := ./
OBJ_DIR := ./

BOARD_OUT_PATH_BIN ?= $(TARGET_DIR)/bin

### For HiSi
CROSS_COMPILE= aarch64-himix100-linux-
LIB_DIR = -L ../../Build/prebuild/lib

#####################################################################################################
CC:= $(CROSS_COMPILE)g++
CXXFLAGS= -g -Wall -std=c++11 -lstdc++ -fPIC
LIBPATH = -lpthread -lm -ldl -fPIC -lrt
LIBPATH+= $(LIB_DIR) -lzbar


SRC = $(wildcard *.cpp)

OBJ = $(SRC:%.cpp=$(OBJ_DIR)/%.o)

INC += -I../../Build/prebuild/include

TARGET := $(TARGET_DIR)/scan_image

$(TARGET): $(OBJ)
	@mkdir -p $(TARGET_DIR)
	$(CC)  -rdynamic -o $@ $^ $(LIBPATH)

$(OBJ): $(OBJ_DIR)/%.o : %.cpp
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CXXFLAGS) $(INC) -o $@ -c $<

all: $(TARGET)

clean:
	rm -rf *.o

.PHONY: clean all

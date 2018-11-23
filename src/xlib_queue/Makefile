
ifndef SRCDIR
	SRCDIR = ./
endif

include ../config.mak
debug ?= yes
OBJS =	$(SRCDIR)comm_debug.o \
		$(SRCDIR)utils_mem.o \
		$(SRCDIR)utils_queue.o

OBJ_NAME = $(LIBS_PATH)/lib_queue.so

CFLAGS	+= -shared -fPIC

ifeq ($(debug),yes)
CFLAGS	+= -Wall -g -ggdb3
else
CFLAGS	+= -Wall -O2 -Os
endif

CINCLUDE	+= -I$(INCLUDE_PATH)
CINCLUDE	+= -I$(SRCDIR)

#CLIB	+= -Wl,-rpath=$(TOPDIR)libs
#CLIB	+= -L$(TOPDIR)../libs

CLIB	+= -lpthread

TARGET	= $(OBJ_NAME)

LINK=$(CC)


include ../rules.mak

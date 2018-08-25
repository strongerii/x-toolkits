
ifndef SRCDIR
	SRCDIR = ./
endif

include ../config.mak
debug ?= yes
OBJS =	$(SRCDIR)utils_shm.o \
		$(SRCDIR)utils_shm_hE.o \
		$(SRCDIR)utils_fifo.o \
		$(SRCDIR)comm_debug.o

OBJ_NAME = $(LIBS_PATH)/lib_fifo.so

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

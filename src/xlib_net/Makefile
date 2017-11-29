
ifndef SRCDIR
	SRCDIR = ./
endif

include ../config.mak

OBJS =	$(SRCDIR)iowatcher.o \
		$(SRCDIR)ipc_net.o \
		$(SRCDIR)ipc_net_ipv6.o \
		$(BASEDIR)xlib_base64/base64.o \
		$(SRCDIR)net_utils.o \
		$(SRCDIR)net_info.o
		
		
OBJ_NAME = $(LIBS_PATH)/xlib_net.so

CFLAGS	+= -shared -fPIC

ifeq ($(debug),yes)
CFLAGS	+= -Wall -O2 -Os -g
else
CFLAGS	+= -Wall -O2 -Os
endif

CINCLUDE	+= -I$(INCLUDE_PATH)
CINCLUDE	+= -I$(SRCDIR)
CINCLUDE	+= -I$(BASEDIR)xlib_base64

#CLIB	+= -Wl,-rpath=$(TOPDIR)libs
#CLIB	+= -L$(TOPDIR)../libs

CLIB    += -lpthread

TARGET	= $(OBJ_NAME)

LINK=$(CC)
	
	
include ../rules.mak
	
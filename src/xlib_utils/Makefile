
ifndef SRCDIR
	SRCDIR = ./
endif

include ../config.mak

OBJS =	$(SRCDIR)log_info.o 
		
OBJ_NAME = $(LIBS_PATH)/xlib_log.so

CFLAGS	+= -shared -fPIC

ifeq ($(debug),yes)
CFLAGS	+= -Wall -O2 -Os -g
else
CFLAGS	+= -Wall -O2 -Os
endif

CINCLUDE	+= -I$(INCLUDE_PATH)
CINCLUDE	+= -I$(SRCDIR)

#CLIB	+= -Wl,-rpath=$(TOPDIR)libs
#CLIB	+= -L$(TOPDIR)../libs

CLIB    += 

TARGET	= $(OBJ_NAME)

LINK=$(CC)
	
	
include ../rules.mak
	
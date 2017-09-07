
ifndef SRCDIR
	SRCDIR = ./
endif

include ../config.mak

SRCS    = $(wildcard $(SRCDIR)*.c)
DIR		= $(notdir $(SRCS))
OBJS 	= $(patsubst %.c,%.o,$(DIR) )

TARGET = $(LIBS_PATH)/xlib_fifo.so

CFLAGS	+= -shared -fPIC

ifeq ($(debug),yes)
CFLAGS	+= -Wall -O2 -Os -g
else
CFLAGS	+= -Wall -O2 -Os
endif

CINCLUDE += -I$(INCLUDE_PATH)
CINCLUDE += -I$(SRCDIR)

#CLIB	+= -Wl,-rpath=$(TOPDIR)libs
#CLIB	+= -L$(TOPDIR)../libs

CLIB    += 

LINK=$(CC)
##LINK=$(CPP)

include ../rules.mak
	
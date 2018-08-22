#
include ../../config.mak

TOPDIR = ../../
SRCDIR = ./

OBJS =	$(SRCDIR)../utils_net.o \
		$(SRCDIR)../comm_debug.o \
		$(SRCDIR)comm_client.o

CFLAGS	+= -Wall -O2 -Os

CINCLUDE  += -I$(TOPDIR)xlib_udp
CINCLUDE  += -I$(TOPDIR)../include
CINCLUDE  += -I$(SRCDIR)../

CLIB += -lpthread
TARGET	= $(SRCDIR)comm_client

LINK=$(CC)

include $(TOPDIR)rules.mak

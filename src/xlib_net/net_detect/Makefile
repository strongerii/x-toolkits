#
include ../../config.mak

TOPDIR = ../../
SRCDIR = ./

OBJS =	$(SRCDIR)../net_detect.o \
		$(SRCDIR)net_main.o

CFLAGS	+= -Wall -O2 -Os

CINCLUDE  += -I$(TOPDIR)../include
CINCLUDE  += -I$(SRCDIR)../

CLIB += -lpthread
TARGET	= $(SRCDIR)net_detect

LINK=$(CC)

include $(TOPDIR)rules.mak

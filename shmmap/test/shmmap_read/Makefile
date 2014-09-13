############################################################################
#
# Makefile for shmmap
#
#
#
# gcc 
#
############################################################################
ifndef TOPDIR
	TOPDIR = ../../
endif
ifndef SRCDIR
	SRCDIR = ./
endif

include $(TOPDIR)../config.mak

OBJS =	$(TOPDIR)sh_tbitstream.o \
		$(TOPDIR)shmmap.o \
		$(SRCDIR)mm_main.o \

CFLAGS	+= -Wall -O2 -Os



CINCLUDE  += -I$(TOPDIR)../shmmap
CINCLUDE  += -I$(TOPDIR)../include

CLIB += -lpthread
TARGET	= $(SRCDIR)shmmap

LINK=$(CC)

include $(TOPDIR)../rules.mak


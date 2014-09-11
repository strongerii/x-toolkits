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
	TOPDIR = ../
endif
ifndef SRCDIR
	SRCDIR = ./
endif

include $(TOPDIR)config.mak

OBJS =	$(TOPDIR)/shmmap_read/sh_tbitstream.o \
		$(TOPDIR)/shmmap_read/shmmap.o \
		$(SRCDIR)mm_main.o \

CFLAGS	+= -Wall -O2 -Os



CINCLUDE  += -I$(TOPDIR)/shmmap_read
CINCLUDE  += -I$(SRCDIR)../include

CLIB += -lpthread
TARGET	= $(SRCDIR)shmmap

LINK=$(CC)

include $(TOPDIR)rules.mak


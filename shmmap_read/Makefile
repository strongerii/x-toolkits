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

OBJS =	$(SRCDIR)sh_tbitstream.o \
		$(SRCDIR)shmmap.o \
		$(SRCDIR)mm_main.o \

CFLAGS	+= -Wall -O2 -Os



CINCLUDE  += -I$(SRCDIR)
CINCLUDE  += -I$(SRCDIR)../include

CLIB += -lpthread
TARGET	= $(SRCDIR)shmmap

LINK=$(CC)

include $(TOPDIR)rules.mak


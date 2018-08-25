############################################################################
#
# Makefile for shmmap
#
#
#
# gcc
#
############################################################################


include ../../config.mak

TOPDIR := ../..
SRCDIR := .

TARGET	= $(SRCDIR)/shmmap

OBJS =	$(SRCDIR)/mm_main.o

CFLAGS	+= -Wall -O2 -Os

CINCLUDE  += -I$(SRCDIR)
CINCLUDE  += -I$(SRCDIR)/../
CINCLUDE  += -I$(TOPDIR)/../include

CLIB += -L$(TOPDIR)/../libs
CLIB += -Wl,-rpath,$(TOPDIR)/../libs

CLIB += -lpthread -l_shmmap

LINK=$(CC)

include ../../rules.mak


#
include ../config.mak

TOPDIR = ../
SRCDIR = ./

OBJS =	$(SRCDIR)utils_net.o \
		$(SRCDIR)utils_sink.o \
		$(SRCDIR)utils_dlinklist.o \
		$(SRCDIR)comm_client_mgr.o \
		$(SRCDIR)comm_parse.o \
		$(SRCDIR)comm_debug.o \
		$(SRCDIR)comm_server.o

CFLAGS	+= -Wall -g

CINCLUDE  += -I$(TOPDIR)xlib_udp
CINCLUDE  += -I$(TOPDIR)../include

CLIB += -lpthread
TARGET	= $(SRCDIR)comm_server

LINK=$(CC)

include $(TOPDIR)rules.mak

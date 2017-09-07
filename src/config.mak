TOPDIR 			:= /home/simon/xlib/
BASEDIR			:= $(TOPDIR)src/
INCLUDE_PATH 	:= $(TOPDIR)include
LIBS_PATH		:= $(TOPDIR)libs

#CROSS	:= arm-none-linux-gnueabi-
CROSS :=
CC	= $(CROSS)gcc
CPP	= $(CROSS)g++
LD	= $(CROSS)ld
AR	= $(CROSS)ar
RM	= rm -f
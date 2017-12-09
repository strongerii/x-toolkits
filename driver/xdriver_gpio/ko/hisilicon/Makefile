
ifeq ($(PARAM_FILE), )
    PARAM_FILE:=../../mpp/Makefile.param
    include $(PARAM_FILE)
endif

CC=$(CROSS)gcc

#EXTRA_CFLAGS += -DHIDEBUG
EXTRA_CFLAGS += -D$(HI_FPGA)

obj-m := hi_gpio.o
higpio-y := hi_gpio.o


.PHONY: all clean ko

all: ko

ko:
	@echo -e "\e[0;32;1m--Compiling 'gpio'...\e[0;36;1m"
	@@echo -e "\e[0m" 
	@make -C $(LINUX_ROOT) M=$(PWD) modules
	@cp hi_gpio.ko $(REL_KO)


clean:
	@make -C $(LINUX_ROOT) M=$(PWD) clean
	@rm -rf $(REL_KO)/hi_gpio.ko
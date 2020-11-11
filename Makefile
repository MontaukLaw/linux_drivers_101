KERNAELDIR := /home/book/driver_linux/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek
CURRENT_PATH := $(shell pwd)
# obj-m := chrdevbase.o led.o newchrled.o dtsled.o dtsgpio.o dtsgpio2.o
obj-m := dtsbeep.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNAELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNAELDIR) M=$(CURRENT_PATH) clean

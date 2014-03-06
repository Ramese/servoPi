obj-m = servoPi_modul.o

SOURCE = /lib/modules/$(uname -r)/build

PWD = $(pwd)

default:
	$(MAKE) -C $(SOURCE) M=$(PWD) modules

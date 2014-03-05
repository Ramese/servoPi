Postup kompilace
--------------

make -C /lib/modules/$(uname -r)/build M=$(pwd) modules

Vlozeni do jadra
--------------

sudo insmod servoPi_modul.ko

Odstraneni z jadra
----------------

sudo rmmod servoPi_modul



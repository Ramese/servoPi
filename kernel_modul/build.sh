#!/bin/bash

export ARCH=arm
export CROSS_COMPILE=/media/Data/KERNEL/tools-master/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-
export INSTALL_MOD_PATH=/media/Data/KERNEL/ModulZkouska/

make -C /media/Data/KERNEL/kompilace/linux-rpi-3.10.y M=$(pwd) modules

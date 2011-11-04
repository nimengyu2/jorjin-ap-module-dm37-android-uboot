#!/bin/sh
export ANDROID_PATH=`pwd`
export PATH=$ANDROID_PATH/../prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin:$PATH
export CROSS_COMPILE=arm-eabi-                                                                                               
export ARCH=arm
export TARGET_PRODUCT=beagleboard 
make distclean && \
make panther_config && \
make 2>&1 |tee u-boot_make.out

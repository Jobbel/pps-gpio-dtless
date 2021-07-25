## pps-gpio-dtless
This Linux kernel module provides a PPS source from a GPIO Pin.

It uses Interrupts and can be configured by module parameters instead of the device tree.

Heavily inspired by https://github.com/mlichvar/pps-gpio-poll and the official pps-gpio.c in the Linux kernel.
## Installation

```
git clone https://github.com/Jobbel/pps-gpio-dtless
```
```
cd ./pps-gpio-dtless
```
Make against currently running kernel.
```
make -C /lib/modules/`uname -r`/build M=$PWD
```
Insert module into the kernel. Linux GPIO numbering applies.
```
sudo insmod ./pps-gpio-dtless.ko gpio=364
```
If you dont want to load the module on every boot you can automate it by creating a config file at /etc/modules-load.d/
## Removal
To remove the installed Module use
```
sudo rmmod ./pps-gpio-dtless.ko
```
inside the cloned directory.

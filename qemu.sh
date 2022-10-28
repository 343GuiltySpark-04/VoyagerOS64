#!/bin/sh

qemu-system-x86_64 -serial file:serial.log -cdrom VoyagerOS.iso --no-reboot

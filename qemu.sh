#!/bin/sh

qemu-system-x86_64 -cpu qemu64,+xsave -M q35 -serial file:serial.log -cdrom VoyagerOS.iso --no-reboot

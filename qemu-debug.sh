#!/bin/sh

qemu-system-x86_64 -S -s -serial file:serial.log -cdrom VoyagerOS.iso  --no-reboot

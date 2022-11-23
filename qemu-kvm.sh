#!/bin/sh

qemu-system-x86_64 -enable-kvm -cpu host,+xsave -M q35 -serial file:serial.log -cdrom VoyagerOS.iso --no-reboot

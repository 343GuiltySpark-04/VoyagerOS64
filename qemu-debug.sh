#!/bin/sh

qemu-system-x86_64 -cpu qemu64,+xsave,+acpi -M q35  -S -s -serial file:serial.log -cdrom VoyagerOS.iso  --no-reboot

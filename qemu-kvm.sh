#!/bin/sh

kvm -cpu qemu64,+xsave,+acpi,rdrand,sse,avx -M q35 -m 256M -serial file:serial.log -cdrom VoyagerOS.iso --no-reboot

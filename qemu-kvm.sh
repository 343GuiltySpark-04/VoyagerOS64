#!/bin/sh

kvm -cpu qemu64,+xsave,+acpi,rdrand,sse,avx -M q35 -serial file:serial.log -cdrom VoyagerOS.iso --no-reboot

#!/bin/sh

qemu-system-x86_64 -qmp tcp:localhost:4444,server,nowait -cpu qemu64,+xsave,+acpi -M q35 -serial file:serial.log -cdrom VoyagerOS.iso --no-reboot
#!/bin/sh

sudo qemu-system-x86_64 -nographic -serial file:serial.log -cdrom VoyagerOS.iso

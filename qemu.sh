#!/bin/sh
set -e
. ./iso.sh

sudo qemu-system-$(./target-triplet-to-arch.sh $HOST) -serial file:serial.log -cdrom VoyagerOS.iso

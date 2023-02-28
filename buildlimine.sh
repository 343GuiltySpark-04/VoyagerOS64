#!/bin/sh

export CC=/usr/bin/gcc
export LD=/usr/bin/ld

rm -Rf limine

git clone https://github.com/limine-bootloader/limine.git --branch=v4.x-branch-binary --depth=1

make -C limine

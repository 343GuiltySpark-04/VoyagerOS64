#!/bin/sh

rm -Rf limine

git clone https://github.com/limine-bootloader/limine.git --branch=v3.0-branch-binary --depth=1

make -C limine

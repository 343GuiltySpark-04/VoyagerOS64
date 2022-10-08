#!/bin/sh

git clone https://github.com/limine-bootloader/limine.git --branch=v4.x-branch-binary --depth=1

cd ./limine

make 

cd ..

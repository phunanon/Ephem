#!/bin/sh
cd build
make -j `nproc --all`
#sudo cp ephem /usr/bin
#!/bin/sh

#Download & install immer
git clone --depth=1 https://github.com/arximboldi/immer.git
mkdir -p immer/build && cd immer/build
cmake .. && sudo make install
cd ../.. && rm -rf immer

#Download & install mimalloc
git clone --depth=1 https://github.com/microsoft/mimalloc.git
mkdir -p mimalloc/out/release && cd mimalloc/out/release
cmake ../.. && sudo make install
cd ../../.. && rm -rf mimalloc

#Build Ephem
[ -d build ] && rm -r build
mkdir build
cd build
cmake ..
cd ..
sh make.sh
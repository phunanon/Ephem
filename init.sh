#!/bin/sh

[ -d build ] && rm -rf build
[ -d immer ] && rm -rf immer
[ -d src/immer ] && rm -rf src/immer
[ -d mimalloc ] && rm -rf mimalloc
mkdir build

#Download immer
git clone --depth 1 --no-checkout https://github.com/arximboldi/immer.git
cd immer
git checkout origin/master -- immer
mkdir ../src/immer
mv immer ../src/immer/immer
cd ..
rm -rf immer

#Download & compile mimalloc
git clone --depth=1 https://github.com/microsoft/mimalloc.git
mkdir -p mimalloc/out/release && cd mimalloc/out/release
cmake ../.. && make -j `nproc --all`
mv libmimalloc.a ../../../build
cd ../../.. && rm -rf mimalloc

#Build Ephem
cd build
cmake ..
cd ..
sh make.sh
#!/bin/sh
if [ ! -f build/ephem ]
then
  sh init.sh
  sh make.sh
fi
./build/ephem
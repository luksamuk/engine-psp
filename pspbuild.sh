#!/bin/bash

mkdir -p build-psp
cd build-psp
psp-cmake -DBUILD_PRX=1 -DENC_PRX=1 ..
exec make

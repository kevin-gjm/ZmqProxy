#!/bin/sh

mkdir -p build
cd build

if [ `uname -o ` = "GNU/Linux"  ] ; then
    echo "linux"
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j8
else
    echo "win"
    cmake -G "MinGW Makefiles" -DCMAKE_SH=CMAKE_SH-NOTFOUND -DCMAKE_BUILD_TYPE=Release ..
    mingw32-make.exe -j8
fi


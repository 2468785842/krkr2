#!/bin/bash

cmake -GNinja -B ./out/linux/debug -DCMAKE_BUILD_TYPE=Debug -DLINUX=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build ./out/linux/debug

if [ ! -f "/usr/lib/libfmod.so" ]; then
    echo "install libfmod.so."
    sudo cp $(find ./out/linux/debug/ -name libfmodL.so) /usr/lib/
    sudo cp $(find ./out/linux/debug/ -name libfmod.so) /usr/lib/
    sudo ln -s /usr/lib/libfmod.so /usr/lib/libfmod.so.6
fi

#!/bin/bash

cmake --preset="Linux Debug Config" -DDISABLE_TEST=ON
cmake --build --preset="Linux Debug Build"

if [ ! -f "/usr/lib/libfmod.so" ]; then
    echo "install libfmod.so."
    sudo cp $(find ./out/linux/debug/ -name libfmodL.so) /usr/lib/
    sudo cp $(find ./out/linux/debug/ -name libfmod.so) /usr/lib/
    sudo ln -s /usr/lib/libfmod.so /usr/lib/libfmod.so.6
fi

#!/bin/bash

cmake -GNinja -B ./out/linux/debug -DCMAKE_BUILD_TYPE=Debug -DLINUX=ON
cmake --build ./out/linux/debug
sudo cp $(find ./out/linux/debug/ -name libfmodL.so) /usr/lib/
sudo cp $(find ./out/linux/debug/ -name libfmod.so) /usr/lib/
sudo ln -s /usr/lib/libfmod.so /usr/lib/libfmod.so.6
#!/bin/bash

cmake --preset="Linux Debug Config" -DDISABLE_TEST=ON
cmake --build --preset="Linux Debug Build"
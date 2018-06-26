#!/bin/bash

set -x
set -e

# Build GoldenCheetah

qmake -recursive CONFIG+=release PREFIX=/usr
make -j$(nproc)
#make INSTALL_ROOT=appdir -j$(nproc) install

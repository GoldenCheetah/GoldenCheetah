#!/bin/bash
set -ev

# D2XX - refresh cache if folder is empty

if [ "$(uname)" == "Linux" ]; then
    if [ -z "$(ls -A D2XX)" ]; then
        mkdir -p D2XX
        wget --no-verbose https://ftdichip.com/wp-content/uploads/2022/07/libftd2xx-x86_64-1.4.27.tgz
        tar xf libftd2xx-x86_64-1.4.27.tgz -C D2XX
    fi
elif [ "$(uname)" == "Darwin" ]; then
    if [ -z "$(ls -A D2XX)" ]; then
        mkdir -p D2XX
        curl -O https://ftdichip.com/wp-content/uploads/2021/05/D2XX1.4.24.zip
        unzip D2XX1.4.24.zip
        hdiutil mount D2XX1.4.24.dmg
        cp /Volumes/dmg/release/build/libftd2xx.1.4.24.dylib D2XX
        cp /Volumes/dmg/release/build/libftd2xx.a D2XX
        cp /Volumes/dmg/release/*.h D2XX
    fi
    sudo cp D2XX/libftd2xx.1.4.24.dylib /usr/local/lib
fi

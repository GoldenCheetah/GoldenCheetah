#!/bin/bash

set -x
set -e

## try early just to check, can delete later
date
brew update
sh travis/install-qt.sh
##brew install $QT
brew install libical
##brew install libusb libusb-compat # no longer needed?
brew install srmio
brew install libsamplerate
rm -rf '/usr/local/include/c++'
brew install r
brew install brewsci/science/lmfit
## Disable KML for now
##brew install --HEAD travis/libkml.rb
sudo chmod -R +w /usr/local
curl -O http://www.ftdichip.com/Drivers/D2XX/MacOSX/D2XX1.2.2.dmg
git clone --branch 0.98 https://github.com/kypeli/kQOAuth.git kQOAuth-0.98
cd kQOAuth-0.98
CC=clang CXX=clang++ /usr/local/opt/$QT_PATH/bin/qmake -makefile -recursive QMAKE_CXXFLAGS_WARN_ON+="-Wno-unused-private-field -Wno-c++11-narrowing"
CC=clang CXX=clang++ make -j2 qmake_all
CC=clang CXX=clang++ sudo make install
cd ..
hdiutil mount D2XX1.2.2.dmg

#!/bin/bash

cat ${0}

# Update brew
brew update

# Install QT
brew install qt$QT

# Install libical support
brew install libical

# Install libusb support
brew install libusb libusb-compat

# Install SRM support
brew install srmio

# Install libmkl support
brew install --HEAD https://raw.github.com/gcoco/CI-for-GoldenCheetah/master/libkml.rb

# Allo write to brew files for later deployment
sudo chmod -R +w /usr/local

# Install qwtplot3d support
git clone --depth 1 https://github.com/sintegrial/qwtplot3d.git qwtplot3d
cd qwtplot3d
CC=clang CXX=clang++ /usr/local/opt/qt$QT/bin/qmake -makefile -recursive QMAKE_CXXFLAGS_WARN_ON+=-Wno-unused-private-field
CC=clang CXX=clang++ make -j2 
cd ..

# Install kQOAuth support
git clone --branch 0.98 https://github.com/kypeli/kQOAuth.git kQOAuth-0.98
cd kQOAuth-0.98
CC=clang CXX=clang++ /usr/local/opt/qt$QT/bin/qmake -makefile -recursive QMAKE_CXXFLAGS_WARN_ON+=-Wno-unused-private-field
CC=clang CXX=clang++ make -j2 qmake_all 
CC=clang CXX=clang++ sudo make install
cd ..

# Install D2XX support
curl -O http://www.ftdichip.com/Drivers/D2XX/MacOSX/D2XX1.2.2.dmg
hdiutil mount D2XX1.2.2.dmg
mkdir D2XX
cp /Volumes/release/D2XX/Object/10.5-10.7/x86_64/libftd2xx.1.2.2.dylib D2XX
sudo cp /Volumes/release/D2XX/Object/10.5-10.7/x86_64/libftd2xx.1.2.2.dylib /usr/local/lib
cp /Volumes/release/D2XX/bin/*.h D2XX

exit

#!/bin/bash
set -ev
## try early just to check, can delete later
date
brew update
brew install qt5
brew install libical
brew install libusb libusb-compat
brew install srmio
brew install libsamplerate
rm -rf '/usr/local/include/c++'
brew install r
## Disable KML for now
## brew install --HEAD travis/libkml.rb
sudo chmod -R +w /usr/local
curl -O https://www.ftdichip.com/Drivers/D2XX/MacOSX/D2XX1.2.2.dmg
hdiutil mount D2XX1.2.2.dmg
exit

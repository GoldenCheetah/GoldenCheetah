#!/bin/bash
set -ev

## try early just to check, can delete later
date
brew update
brew unlink python@2 # to avoid conflicts with qt/libical dependence on python
brew upgrade python3 # to get 3.7.7
brew upgrade qt5 # to get 5.14.2
brew install libical
brew upgrade libusb
brew install srmio
brew install libsamplerate
rm -rf '/usr/local/include/c++'
brew install r
## Disable KML for now
## brew install --HEAD travis/libkml.rb
sudo chmod -R +w /usr/local
curl -O https://www.ftdichip.com/Drivers/D2XX/MacOSX/D2XX1.2.2.dmg
hdiutil mount D2XX1.2.2.dmg

# AWS client to upload binaries to S3 bucket
brew install awscli
aws --version

exit

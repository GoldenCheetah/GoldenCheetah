#!/bin/bash
set -ev

## try early just to check, can delete later
date
brew update
brew unlink python@2 # to avoid conflicts with qt/libical dependence on python
brew upgrade python3 # to get 3.7.7
brew upgrade qt5 # to get 5.14.2
brew install gsl
brew install libical
brew upgrade libusb
brew install srmio
brew install libsamplerate
rm -rf '/usr/local/include/c++'
brew install r
## Disable KML for now
## brew install --HEAD travis/libkml.rb
sudo chmod -R +w /usr/local

# D2XX - refresh cache if folder is empty
if [ -z "$(ls -A D2XX)" ]; then
    curl -O https://www.ftdichip.com/Drivers/D2XX/MacOSX/D2XX1.2.2.dmg
    hdiutil mount D2XX1.2.2.dmg
    cp /Volumes/release/D2XX/Object/10.5-10.7/x86_64/libftd2xx.1.2.2.dylib D2XX
    cp /Volumes/release/D2XX/bin/*.h D2XX
fi
sudo cp D2XX/libftd2xx.1.2.2.dylib /usr/local/lib


# AWS client to upload binaries to S3 bucket
brew install awscli
aws --version

exit

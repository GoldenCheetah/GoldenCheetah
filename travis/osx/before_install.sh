#!/bin/bash
set -ev

date
# update to use latest Qt5 version
brew update

brew install qt5
brew install bison@2.7
brew install gsl
brew install libical
brew upgrade libusb
brew install libsamplerate
rm -rf '/usr/local/include/c++'
sudo chmod -R +w /usr/local

# R 4.1.1
curl -L -O https://cran.r-project.org/bin/macosx/base/R-4.1.1.pkg
sudo installer -pkg R-4.1.1.pkg -target /
R --version

# SRMIO
curl -L -O https://github.com/rclasen/srmio/archive/v0.1.1git1.tar.gz
tar xf v0.1.1git1.tar.gz
cd srmio-0.1.1git1
sh genautomake.sh
./configure --disable-shared --enable-static
make -j3 --silent
sudo make install
cd ..

# D2XX - refresh cache if folder is empty
if [ -z "$(ls -A D2XX)" ]; then
    curl -O https://ftdichip.com/wp-content/uploads/2021/05/D2XX1.4.24.zip
    unzip D2XX1.4.24.zip
    hdiutil mount D2XX1.4.24.dmg
    cp /Volumes/dmg/release/build/libftd2xx.1.4.24.dylib D2XX
    cp /Volumes/dmg/release/build/libftd2xx.a D2XX
    cp /Volumes/dmg/release/*.h D2XX
fi
sudo cp D2XX/libftd2xx.1.4.24.dylib /usr/local/lib

# VLC
if [[ -z "$(ls -A VLC)" ]]; then
    curl -O http://download.videolan.org/pub/videolan/vlc/3.0.8/macosx/vlc-3.0.8.dmg
    hdiutil mount vlc-3.0.8.dmg
    cp -R "/Volumes/VLC media player/VLC.app/Contents/MacOS/include" VLC/include
    cp -R "/Volumes/VLC media player/VLC.app/Contents/MacOS/lib" VLC/lib
    cp -R "/Volumes/VLC media player/VLC.app/Contents/MacOS/plugins" VLC/plugins
    rm -f VLC/plugins/plugins.dat
fi
sudo cp VLC/lib/libvlc*.dylib /usr/local/lib

# AWS client to upload binaries to S3 bucket
curl -O https://awscli.amazonaws.com/AWSCLIV2.pkg
sudo installer -pkg AWSCLIV2.pkg -target /
aws --version

exit

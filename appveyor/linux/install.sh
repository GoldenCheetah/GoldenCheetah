#!/bin/bash
set -ev

sudo apt-get update -qq
sudo apt-get install -qq flex libpulse-dev
sudo apt-get install -qq libglu1-mesa-dev
sudo apt-get install -qq libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
sudo apt-get install -qq libsamplerate0-dev
sudo apt-get install -qq libical-dev

# Add VLC 3
sudo apt-get install -y vlc libvlc-dev libvlccore-dev

# R 4.0
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys E298A3A825C0D65DFD57CBB651716619E084DAB9
sudo add-apt-repository -y "deb https://cloud.r-project.org/bin/linux/ubuntu jammy-cran40/"
sudo apt-get update -qq
sudo apt-get install -qq r-base-dev
R --version

# D2XX - refresh cache if folder is empty
if [ -z "$(ls -A D2XX)" ]; then
    mkdir -p D2XX
    wget --no-verbose https://ftdichip.com/wp-content/uploads/2022/07/libftd2xx-x86_64-1.4.27.tgz
    tar xf libftd2xx-x86_64-1.4.27.tgz -C D2XX
fi

# SRMIO
if [ -z "$(ls -A srmio)" ]; then
    git clone https://github.com/rclasen/srmio.git
    cd srmio
    sh genautomake.sh
    ./configure --disable-shared --enable-static
    make --silent -j2
    cd ..
fi
cd srmio
sudo make install
cd ..

# LIBUSB
sudo apt-get install -qq libusb-1.0-0-dev libudev-dev

# Add Python 3.7 and SIP
sudo add-apt-repository -y ppa:deadsnakes/ppa
sudo apt-get update -qq
sudo apt-get install -qq python3.7-dev python3.7-distutils
python3.7 --version
if [ -z "$(ls -A sip-4.19.8)" ]; then
    wget --no-verbose https://sourceforge.net/projects/pyqt/files/sip/sip-4.19.8/sip-4.19.8.tar.gz
    tar xf sip-4.19.8.tar.gz
    cd sip-4.19.8
    python3.7 configure.py --incdir=/usr/include/python3.7
    make -j2
    cd ..
fi
cd sip-4.19.8
sudo make install
cd ..

# GSL
sudo apt-get -qq install libgsl-dev

# Install fuse2 required to run older AppImages, and patchelf to fix QtWebEngineProcess
sudo add-apt-repository -y universe
sudo apt install libfuse2 patchelf

exit

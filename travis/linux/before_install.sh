#!/bin/bash
set -ev

# install Bison 3.5 from focal
wget http://mirrors.kernel.org/ubuntu/pool/main/b/bison/bison_3.5.1+dfsg-1_amd64.deb
sudo dpkg -i bison_3.5.1+dfsg-1_amd64.deb

# Add recent Qt dependency ppa, update on a newer qt version.
sudo add-apt-repository -y ppa:beineri/opt-qt-5.14.2-bionic
sudo apt-get update -qq
sudo apt-get install -qq qt5-default qt514base qt514tools qt514serialport\
 qt514svg qt514multimedia qt514connectivity qt514webengine qt514charts-no-lgpl\
 qt514networkauth-no-lgpl qt514translations

sudo apt-get install -qq libglu1-mesa-dev
sudo apt-get install -qq libsamplerate0-dev
sudo apt-get install -qq libkml-dev
sudo apt-get install -qq libical-dev

# Add VLC 3
sudo add-apt-repository -y ppa:jonathonf/vlc-3
sudo add-apt-repository -y ppa:jonathonf/ffmpeg-4
sudo apt-get update -qq
sudo apt-get install -y vlc libvlc-dev libvlccore-dev

# R 4.0
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys E298A3A825C0D65DFD57CBB651716619E084DAB9
sudo add-apt-repository -y "deb https://cloud.r-project.org/bin/linux/ubuntu bionic-cran40/"
sudo apt-get update -qq
sudo apt-get install r-base-dev
R --version

# D2XX - refresh cache if folder is empty
if [ -z "$(ls -A D2XX)" ]; then
    wget --no-verbose http://www.ftdichip.com/Drivers/D2XX/Linux/libftd2xx-x86_64-1.3.6.tgz
    tar xf libftd2xx-x86_64-1.3.6.tgz -C D2XX
fi

# SRMIO
wget --no-verbose https://github.com/rclasen/srmio/archive/v0.1.1git1.tar.gz
tar xf v0.1.1git1.tar.gz
cd srmio-0.1.1git1
sh genautomake.sh
./configure --disable-shared --enable-static
make --silent -j3
sudo make install
cd ${TRAVIS_BUILD_DIR}

# LIBUSB
sudo apt-get install -qq libusb-1.0-0-dev libudev-dev

# Add Python 3.7 and SIP
sudo add-apt-repository -y ppa:deadsnakes/ppa
sudo apt-get update -qq
sudo apt-get install -qq python3.7-dev
python3.7 --version
wget --no-verbose https://sourceforge.net/projects/pyqt/files/sip/sip-4.19.8/sip-4.19.8.tar.gz
tar xf sip-4.19.8.tar.gz
cd sip-4.19.8
python3.7 configure.py
make
sudo make install
cd ${TRAVIS_BUILD_DIR}

# GSL
sudo apt-get -qq install libgsl-dev

# AWS S3 client to upload binaries
curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o "awscliv2.zip"
unzip -qq awscliv2.zip
sudo ./aws/install
aws --version

exit

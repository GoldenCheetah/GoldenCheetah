#!/bin/bash
set -ev
# Add recent Qt dependency ppa, update on a newer qt version.
sudo add-apt-repository -y ppa:beineri/opt-qt597-trusty
sudo apt-get update -qq
sudo apt-get install -qq qt5-default qt59base qt59tools qt59serialport qt59svg\
 qt59multimedia qt59connectivity qt59webengine qt59charts-no-lgpl qt59networkauth-no-lgpl

sudo apt-get install -qq libglu1-mesa-dev libgstreamer0.10-0 libgstreamer-plugins-base0.10-0
sudo apt-get install -qq libssl-dev libsamplerate0-dev libpulse-dev
sudo apt-get install -qq libical-dev libkml-dev libboost-all-dev

# Add VLC 2.2.8
sudo add-apt-repository -y ppa:jonathonf/vlc
sudo apt-get update -qq
sudo apt-get install -qq libvlc-dev libvlccore-dev

# R 3.5
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys E084DAB9
sudo add-apt-repository -y "deb https://cloud.r-project.org/bin/linux/ubuntu trusty-cran35/"
sudo apt-get update -qq
sudo apt-get install r-base-dev
R --version

# D2XX
wget http://www.ftdichip.com/Drivers/D2XX/Linux/libftd2xx-x86_64-1.3.6.tgz
mkdir D2XX
tar xf libftd2xx-x86_64-1.3.6.tgz -C D2XX

# SRMIO
wget http://www.zuto.de/project/files/srmio/srmio-0.1.1~git1.tar.gz
tar xf srmio-0.1.1~git1.tar.gz
cd srmio-0.1.1~git1
./configure --disable-shared --enable-static
make --silent -j3
sudo make install
cd ${TRAVIS_BUILD_DIR}

# LIBUSB
#sudo apt-get install -qq libusb-dev libudev-dev
sudo apt-get install -qq libusb-1.0-0-dev libudev-dev
wget https://sourceforge.net/projects/libusb/files/libusb-compat-0.1/libusb-compat-0.1.5/libusb-compat-0.1.5.tar.bz2/download -O libusb-compat-0.1.5.tar.bz2
tar xf libusb-compat-0.1.5.tar.bz2
cd libusb-compat-0.1.5
./configure --disable-shared --enable-static
make --silent -j3
sudo make install
cd ${TRAVIS_BUILD_DIR}

# Add Python 3.6 and SIP
sudo add-apt-repository -y ppa:jonathonf/python-3.6
sudo apt-get update -qq
sudo apt-get install -qq python3.6-dev
python3.6 --version
wget https://sourceforge.net/projects/pyqt/files/sip/sip-4.19.8/sip-4.19.8.tar.gz
tar xf sip-4.19.8.tar.gz
cd sip-4.19.8
python3.6 configure.py
make
sudo make install
cd ${TRAVIS_BUILD_DIR}

exit

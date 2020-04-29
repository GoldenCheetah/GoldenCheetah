#!/bin/bash
set -ev
# Add recent Qt dependency ppa, update on a newer qt version.
sudo add-apt-repository -y ppa:beineri/opt-qt-5.14.2-xenial
sudo apt-get update -qq
sudo apt-get install -qq qt5-default qt514base qt514tools qt514serialport\
 qt514svg qt514multimedia qt514connectivity qt514webengine qt514charts-no-lgpl\
 qt514networkauth-no-lgpl qt514translations

sudo apt-get install -qq libglu1-mesa-dev libgstreamer0.10-0 libgstreamer-plugins-base0.10-0
sudo apt-get install -y --allow-downgrades libpulse0=1:8.0-0ubuntu3
sudo apt-get install -qq libssl-dev libsamplerate0-dev libpulse-dev
sudo apt-get install -qq libical-dev libkml-dev libboost-all-dev

# Add OpenSSL 1.1.1 (required by Qt 5.14)
wget https://www.openssl.org/source/openssl-1.1.1d.tar.gz
tar xf openssl-1.1.1d.tar.gz
cd openssl-1.1.1d
./Configure shared --prefix=/usr --openssldir=/usr/lib/ssl --libdir=lib no-idea no-mdc2 no-rc5 no-zlib no-ssl3 enable-ec_nistp_64_gcc_128 linux-x86_64
make -j4
sudo cp libssl.so.1.1 libcrypto.so.1.1 /usr/local/lib/
sudo ldconfig
cd ..

# Add VLC 2.2.2
sudo apt-get install -qq vlc libvlc-dev libvlccore-dev

# R 3.6
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys E298A3A825C0D65DFD57CBB651716619E084DAB9
sudo add-apt-repository -y "deb https://cloud.r-project.org/bin/linux/ubuntu xenial-cran35/"
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
sudo apt-get install -qq libusb-1.0-0-dev libudev-dev

# Add Python 3.6 and SIP
sudo add-apt-repository -y ppa:deadsnakes/ppa
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

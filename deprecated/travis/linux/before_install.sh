#!/bin/bash
set -ev

# Install qt5.15
sudo apt-get update -qq
sudo apt-get install -qq qtbase5-dev qt5-qmake qtbase5-dev-tools qttools5-dev qttranslations5-l10n
sudo apt-get install -qq qtmultimedia5-dev qtconnectivity5-dev qtwebengine5-dev qtpositioning5-dev
sudo apt-get install -qq libqt5charts5-dev libqt5serialport5-dev libqt5webchannel5-dev libqt5svg5-dev libqt5opengl5-dev
qmake --version

sudo apt-get install -qq libglu1-mesa-dev
sudo apt-get install -qq libsamplerate0-dev
sudo apt-get install -qq libical-dev

# Add VLC 3
sudo apt-get install -y vlc libvlc-dev libvlccore-dev

# R 4.0
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys E298A3A825C0D65DFD57CBB651716619E084DAB9
sudo add-apt-repository -y "deb https://cloud.r-project.org/bin/linux/ubuntu jammy-cran40/"
sudo apt-get update -qq
sudo apt-get install r-base-dev
R --version

# D2XX - refresh cache if folder is empty
if [ -z "$(ls -A D2XX)" ]; then
    wget --no-verbose https://ftdichip.com/wp-content/uploads/2022/07/libftd2xx-x86_64-1.4.27.tgz
    tar xf libftd2xx-x86_64-1.4.27.tgz -C D2XX
fi

# SRMIO
git clone https://github.com/rclasen/srmio.git
cd srmio
sh genautomake.sh
./configure --disable-shared --enable-static
make --silent -j3
sudo make install
cd ${TRAVIS_BUILD_DIR}

# LIBUSB
sudo apt-get install -qq libusb-1.0-0-dev libudev-dev

# Add Python 3.7 and SIP
sudo apt-get install python3
sudo apt-get install python3-pip
pip install --upgrade pip
pip install -r src/Python/requirements.txt

cd ${TRAVIS_BUILD_DIR}

# GSL
sudo apt-get -qq install libgsl-dev

# GHR to upload binaries to GitHub releases
wget --no-verbose https://github.com/tcnksm/ghr/releases/download/v0.16.2/ghr_v0.16.2_linux_amd64.tar.gz
tar xf ghr_v0.16.2_linux_amd64.tar.gz
mv ghr_v0.16.2_linux_amd64 ghr

# Install fuse2 required to run older AppImages, and patchelf to fix QtWebEngineProcess
sudo add-apt-repository -y universe
sudo apt install libfuse2 patchelf

exit

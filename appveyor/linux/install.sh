#!/bin/bash
set -ev

sudo apt-get update -qq
sudo apt-get install -qq flex libpulse-dev
sudo apt-get install -qq libglu1-mesa-dev libxcb-cursor-dev
sudo apt-get install -qq libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
sudo apt-get install -qq libsamplerate0-dev
sudo apt-get install -qq libical-dev

# R 4.0
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys E298A3A825C0D65DFD57CBB651716619E084DAB9
sudo add-apt-repository -y "deb https://cloud.r-project.org/bin/linux/ubuntu jammy-cran40/"
sudo apt-get update -qq
sudo apt-get install -qq r-base-dev
R --version

# D2XX - refresh cache if folder is empty
bash appveyor/common/install_d2xx.sh


# SRMIO
bash appveyor/common/install_srmio.sh

# LIBUSB
sudo apt-get install -qq libusb-1.0-0-dev libudev-dev

# Add Python 3.7 and SIP
sudo add-apt-repository -y ppa:deadsnakes/ppa
sudo apt-get update -qq
sudo apt-get install -qq python3.7-dev python3.7-distutils
python3.7 --version

bash appveyor/common/install_sip.sh

# GSL
sudo apt-get -qq install libgsl-dev

# Install fuse2 required to run older AppImages, and patchelf to fix QtWebEngineProcess
sudo add-apt-repository -y universe
sudo apt install libfuse2 patchelf

exit

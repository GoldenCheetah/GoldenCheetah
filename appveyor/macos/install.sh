#!/bin/bash
set -ev

date
# Don't update or cleanup
export HOMEBREW_NO_AUTO_UPDATE=1
export HOMEBREW_NO_INSTALL_CLEANUP=1

brew install bison
brew install gsl
brew install libical
brew upgrade libusb
brew install libsamplerate
rm -rf '/opt/homebrew/include/c++'
sudo chmod -R +w /opt/homebrew

# R 4.1.1

if [ ! -f R-4.1.1-arm64.pkg ]; then
    curl -L -O https://cran.r-project.org/bin/macosx/big-sur-arm64/base/R-4.1.1-arm64.pkg
    sudo installer -pkg R-4.1.1-arm64.pkg -target /
    R --version
fi

# SRMIO
if [ -z "$(ls -A srmio)" ]; then
    git clone https://github.com/rclasen/srmio.git
    cd srmio
    sh genautomake.sh
    ./configure --disable-shared --enable-static --prefix=/opt/homebrew
    make -j2 --silent
    cd ..
fi
cd srmio
sudo make install
cd ..

# D2XX - refresh cache if folder is empty
if [ -z "$(ls -A D2XX)" ]; then
    mkdir -p D2XX
    curl -O https://ftdichip.com/wp-content/uploads/2021/05/D2XX1.4.24.zip
    unzip D2XX1.4.24.zip
    hdiutil mount D2XX1.4.24.dmg
    cp /Volumes/dmg/release/build/libftd2xx.1.4.24.dylib D2XX
    cp /Volumes/dmg/release/build/libftd2xx.a D2XX
    cp /Volumes/dmg/release/*.h D2XX
fi
sudo cp D2XX/libftd2xx.1.4.24.dylib /opt/homebrew/lib

# Python ${PYTHON_VERSION} for embedding (system Python is too old for sip-tools)
brew install python@${PYTHON_VERSION}
export PATH="/opt/homebrew/opt/python@${PYTHON_VERSION}/bin:$PATH"
python3 --version
# Upgrade pip to ensure you have the latest version
python3 -m pip install --upgrade pip

exit

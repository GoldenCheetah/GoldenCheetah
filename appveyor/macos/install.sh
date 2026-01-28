#!/bin/bash
set -ev

date
# Don't update or cleanup
export HOMEBREW_NO_AUTO_UPDATE=1
export HOMEBREW_NO_INSTALL_CLEANUP=1

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
if [ -z "$(ls -A srmio)" ]; then
    git clone https://github.com/rclasen/srmio.git
    cd srmio
    sh genautomake.sh
    ./configure --disable-shared --enable-static
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
sudo cp D2XX/libftd2xx.1.4.24.dylib /usr/local/lib

# Python ${PYTHON_VERSION} for embedding (system Python is too old for sip-tools)
brew install python@${PYTHON_VERSION}
export PATH="/usr/local/opt/python@${PYTHON_VERSION}/bin:$PATH"
python3 --version

# Note: Python packages are installed in appveyor.yml via pip install -r requirements.txt
# They go into the brew Python framework's site-packages, which is copied in after_build.sh

exit

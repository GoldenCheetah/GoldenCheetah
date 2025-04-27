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
    mkdir -p D2XX
    curl -O https://ftdichip.com/wp-content/uploads/2021/05/D2XX1.4.24.zip
    unzip D2XX1.4.24.zip
    hdiutil mount D2XX1.4.24.dmg
    cp /Volumes/dmg/release/build/libftd2xx.1.4.24.dylib D2XX
    cp /Volumes/dmg/release/build/libftd2xx.a D2XX
    cp /Volumes/dmg/release/*.h D2XX
fi
sudo cp D2XX/libftd2xx.1.4.24.dylib /usr/local/lib

# Python 3.7.8
curl -O https://www.python.org/ftp/python/3.7.9/python-3.7.9-macosx10.9.pkg
sudo installer -pkg python-3.7.9-macosx10.9.pkg -target /

python3.7 --version
python3.7-config --prefix

# Python mandatory packages - refresh cache if folder is empty
if [ -z "$(ls -A site-packages)" ]; then
    mkdir -p site-packages
    python3.7 -m pip install -q -r src/Python/requirements.txt -t site-packages
fi

# Python SIP
curl -k -L -O https://sourceforge.net/projects/pyqt/files/sip/sip-4.19.8/sip-4.19.8.tar.gz
tar xf sip-4.19.8.tar.gz
cd sip-4.19.8
python3.7 configure.py
make -j4
sudo make install
cd ..

exit

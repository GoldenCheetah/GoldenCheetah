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
bash appveyor/common/install_srmio.sh

# D2XX - refresh cache if folder is empty
bash appveyor/common/install_d2xx.sh

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
bash appveyor/common/install_sip.sh

exit

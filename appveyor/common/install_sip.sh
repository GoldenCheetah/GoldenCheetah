#!/bin/bash
set -ev

# Python SIP
# Assumes python3.7 is available in path

if [ -z "$(ls -A sip-4.19.8)" ]; then
    
    if [ "$(uname)" == "Linux" ]; then
         wget --no-verbose https://sourceforge.net/projects/pyqt/files/sip/sip-4.19.8/sip-4.19.8.tar.gz
         tar xf sip-4.19.8.tar.gz
         cd sip-4.19.8
         python3.7 configure.py --incdir=/usr/include/python3.7
    elif [ "$(uname)" == "Darwin" ]; then
         curl -k -L -O https://sourceforge.net/projects/pyqt/files/sip/sip-4.19.8/sip-4.19.8.tar.gz
         tar xf sip-4.19.8.tar.gz
         cd sip-4.19.8
         python3.7 configure.py
    fi
    
    make -j2
    cd ..
fi

cd sip-4.19.8
sudo make install
cd ..

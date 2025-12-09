#!/bin/bash
set -ev

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

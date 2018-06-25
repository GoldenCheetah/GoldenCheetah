#!/bin/bash

set -x
set -e


# Add recent Qt dependency ppa
# Update this file on a newer qt version.
sudo add-apt-repository ppa:beineri/opt-qt595-trusty -y
sudo apt update
sudo apt install -y qt59-meta-minimal qt59base qt59tools qt59serialport qt59svg\
 qt59multimedia qt59script qt59connectivity flex bison git

source /opt/qt59/bin/qt59-env.sh


#!/bin/bash
set -ev
export PATH=/opt/qt59/bin:$PATH
qmake -recursive
make --silent -j3 || make
exit

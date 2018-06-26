#!/bin/bash

set -x
set -e

### This script should be run from GoldenCheetah root directory after build
if [ ! -x src/GoldenCheetah ]
then echo "Build GoldenCheetah and execute from distribution root"; exit 1
fi

### Create AppDir and start populating
mkdir -p appdir/bin
# Executable
cp src/GoldenCheetah appdir/bin/GoldenCheetah
# Desktop file
cat >appdir/GoldenCheetah.desktop <<EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=GoldenCheetah
Comment=Cycling Power Analysis Software.
Exec=GoldenCheetah
Icon=gc
EOF
# Icon
cp ./src/Resources/images/gc.png appdir/

### Download current version of linuxdeployqt
wget -c https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod a+x linuxdeployqt-continuous-x86_64.AppImage


### Download current version of linuxdeployqt
wget -c https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod a+x linuxdeployqt-continuous-x86_64.AppImage

### Deploy to AppDir and generate AppImage
# -qmake=path-to-qmake-used-for-build option is necessary if the right qmakei
# version is not first in PATH, check using qmake --version
export VERSION=$(git rev-parse --short HEAD) # linuxdeployqt uses this for naming the file
./linuxdeployqt-continuous-x86_64.AppImage appdir/GoldenCheetah.desktop -verbose=2 -bundle-non-qt-libs
./linuxdeployqt-continuous-x86_64.AppImage appdir/GoldenCheetah.desktop -appimage

### Minimum Test - Result is ./GoldenCheetah-x86_64.AppImage
if [ ! -x ./GoldenCheetah-${VERSION}-x86_64.AppImage ]
then echo "AppImage not generated, check the errors"; exit 1
fi
./GoldenCheetah-${VERSION}-x86_64.AppImage --version

curl --upload-file "${TRAVIS_BUILD_DIR}/GoldenCheetah-${VERSION}-x86_64.AppImage" https://transfer.sh/GoldenCheetah-${VERSION}-x86_64.AppImage

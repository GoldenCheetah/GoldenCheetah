#!/bin/sh

### This script should be run from GoldenCheetah root directory after build
if [ ! -x src/GoldenCheetah ]
then echo "Build GoldenCheetah and execute from distribution root"; exit 1
fi

### Create AppDir and start populating
mkdir -p appdir
# Executable
cp src/GoldenCheetah appdir
# Desktop file
cat >appdir/GoldenCheetah.desktop <<EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=GoldenCheetah
Comment=Cycling Power Analysis Software.
Exec=GoldenCheetah
Icon=gc
Categories=Science;Sports;
EOF
# Icon
cp ./src/Resources/images/gc.png appdir/

### Add OpenSSL libs
mkdir appdir/lib
cp /usr/lib/x86_64-linux-gnu/libssl.so appdir/lib
cp /usr/lib/x86_64-linux-gnu/libcrypto.so appdir/lib

### Download current version of linuxdeployqt
wget -c https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod a+x linuxdeployqt-continuous-x86_64.AppImage

### Deploy to appdir and generate AppImage
# -qmake=path-to-qmake-used-for-build option is necessary if the right qmakei
# version is not first in PATH, check using qmake --version
./linuxdeployqt-continuous-x86_64.AppImage appdir/GoldenCheetah -verbose=2 -bundle-non-qt-libs -exclude-libs=libqsqlmysql,libqsqlpsql,libnss3,libnssutil3,libxcb-dri3.so.0 -appimage

### Cleanup
rm linuxdeployqt-continuous-x86_64.AppImage
rm -rf appdir

### Minimum Test - Result is ./GoldenCheetah-x86_64.AppImage
if [ ! -x ./GoldenCheetah-x86_64.AppImage ]
then echo "AppImage not generated, check the errors"; exit 1
fi
./GoldenCheetah-x86_64.AppImage --version
exit

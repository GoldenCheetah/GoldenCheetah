#!/bin/bash
set -ev
export PATH=/opt/qt59/bin:$PATH
export LD_LIBRARY_PATH=/opt/qt59/lib/x86_64-linux-gnu:/opt/qt59/lib:$LD_LIBRARY_PATH

### This script should be run from GoldenCheetah src directory after build
cd src
if [ ! -x ./GoldenCheetah ]
then echo "Build GoldenCheetah and execute from distribution src"; exit 1
fi
echo "Checking GoldenCheetah.app can execute"
./GoldenCheetah --help

### Create AppDir and start populating
mkdir -p appdir
# Executable
cp GoldenCheetah appdir
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
cp Resources/images/gc.png appdir/

### Add OpenSSL libs
mkdir appdir/lib
cp /usr/lib/x86_64-linux-gnu/libssl.so appdir/lib
cp /usr/lib/x86_64-linux-gnu/libcrypto.so appdir/lib

### Download current version of linuxdeployqt
wget --no-verbose -c https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod a+x linuxdeployqt-continuous-x86_64.AppImage

### Deploy to appdir and generate AppImage
# -qmake=path-to-qmake-used-for-build option is necessary if the right qmakei
# version is not first in PATH, check using qmake --version
./linuxdeployqt-continuous-x86_64.AppImage appdir/GoldenCheetah -verbose=2 -bundle-non-qt-libs -exclude-libs=libqsqlmysql,libqsqlpsql,libnss3,libnssutil3,libxcb-dri3.so.0 -appimage

### Cleanup
rm linuxdeployqt-continuous-x86_64.AppImage
rm -rf appdir

if [ ! -x ./GoldenCheetah*.AppImage ]
then echo "AppImage not generated, check the errors"; exit 1
fi
echo "Renaming AppImage file to branch and build number ready for deploy"
mv GoldenCheetah*.AppImage $FINAL_NAME
ls -l $FINAL_NAME
### Minimum Test
./$FINAL_NAME --version
### upload for testing
curl --upload-file $FINAL_NAME https://transfer.sh/$FINAL_NAME
cd ${TRAVIS_BUILD_DIR}
exit

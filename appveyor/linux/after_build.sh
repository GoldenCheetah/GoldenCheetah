#!/bin/bash
set -ev

### This script should be run from GoldenCheetah src directory after build
cd src
if [ ! -x ./GoldenCheetah ]
then echo "Build GoldenCheetah and execute from distribution src"; exit 1
fi

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

### Download current version of linuxdeployqt
wget --no-verbose -c https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod a+x linuxdeployqt-continuous-x86_64.AppImage

### Deploy to appdir
./linuxdeployqt-continuous-x86_64.AppImage appdir/GoldenCheetah -verbose=2 -bundle-non-qt-libs -exclude-libs=libqsqlmysql,libqsqlpsql,libqsqlmimer,libqsqlodbc,libnss3,libnssutil3,libxcb-dri3.so.0 -unsupported-allow-new-glibc

# Add Python and core modules
wget --no-verbose https://github.com/niess/python-appimage/releases/download/python3.7/python3.7.17-cp37-cp37m-manylinux1_x86_64.AppImage
chmod +x python3.7.17-cp37-cp37m-manylinux1_x86_64.AppImage
./python3.7.17-cp37-cp37m-manylinux1_x86_64.AppImage --appimage-extract
rm -f python3.7.17-cp37-cp37m-manylinux1_x86_64.AppImage
export PATH="$(pwd)/squashfs-root/usr/bin:$PATH"
pip install --upgrade pip
pip install -q -r Python/requirements.txt
mv squashfs-root/usr appdir/usr
mv squashfs-root/opt appdir/opt
rm -rf squashfs-root

# Generate AppImage
wget --no-verbose "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
# Fix RPATH on QtWebEngineProcess
patchelf --set-rpath '$ORIGIN/../lib' appdir/libexec/QtWebEngineProcess
chmod a+x appimagetool-x86_64.AppImage
./appimagetool-x86_64.AppImage appdir

### Cleanup
rm linuxdeployqt-continuous-x86_64.AppImage
rm appimagetool-x86_64.AppImage
rm -rf appdir

if [ ! -x ./GoldenCheetah*.AppImage ]
then echo "AppImage not generated, check the errors"; exit 1
fi

echo "Renaming AppImage file to version number ready for deploy"
mv GoldenCheetah*.AppImage ../GoldenCheetah_v3.7_x64.AppImage

exit

#!/bin/bash
set -ev

### This script should be run from GoldenCheetah src directory after build
if [ ! -x ./GoldenCheetah ]
then echo "Build GoldenCheetah and execute from distribution src"; exit 1
fi

qmake --version

echo "Checking GoldenCheetah.app can execute"
./GoldenCheetah --version

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

### Add vlc 3
#mkdir appdir/lib
#cp -r /usr/lib/x86_64-linux-gnu/vlc appdir/lib/vlc
#sudo appdir/lib/vlc/vlc-cache-gen appdir/lib/vlc/plugins

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

# Fix RPATH on QtWebEngineProcess and copy missing resources
patchelf --set-rpath '$ORIGIN/../lib' appdir/libexec/QtWebEngineProcess
cp -r `qmake -v|awk '/Qt/ { print $6 "/../resources" }' -` appdir

# Generate AppImage
wget --no-verbose "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
chmod a+x appimagetool-x86_64.AppImage
./appimagetool-x86_64.AppImage appdir

### Cleanup
rm linuxdeployqt-continuous-x86_64.AppImage
rm appimagetool-x86_64.AppImage
rm -rf appdir

if [ ! -x ./GoldenCheetah-x86_64.AppImage ]
then echo "AppImage not generated, check the errors"; exit 1
fi

echo "Renaming AppImage file to branch and build number ready for deploy"
export FINAL_NAME=GoldenCheetah_v3.7_x64Qt6.AppImage
mv -f GoldenCheetah-x86_64.AppImage $FINAL_NAME
ls -l $FINAL_NAME

### Generate version file with SHA
./$FINAL_NAME --version 2>GCversionLinuxQt6.txt
git log -1 >> GCversionLinuxQt6.txt
echo "SHA256 hash of $FINAL_NAME:" >> GCversionLinuxQt6.txt
shasum -a 256 $FINAL_NAME | cut -f 1 -d ' '  >> GCversionLinuxQt6.txt
cat GCversionLinuxQt6.txt

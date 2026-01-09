#!/bin/bash
set -ev

# Python version configuration - update this when upgrading Python
PYTHON_VERSION="3.11"
PYTHON_APPIMAGE_VERSION="3.11.14"

### This script should be run from GoldenCheetah src directory after build
cd src
if [ ! -x ./GoldenCheetah ]; then
    echo "Build GoldenCheetah and execute from distribution src"
    exit 1
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
PYTHON_APPIMAGE_FILE="python${PYTHON_APPIMAGE_VERSION}-cp${PYTHON_VERSION//./}-cp${PYTHON_VERSION//./}-manylinux_2_28_x86_64.AppImage"
wget --no-verbose "https://github.com/niess/python-appimage/releases/download/python${PYTHON_VERSION}/${PYTHON_APPIMAGE_FILE}"
chmod +x "${PYTHON_APPIMAGE_FILE}"
./"${PYTHON_APPIMAGE_FILE}" --appimage-extract
rm -f "${PYTHON_APPIMAGE_FILE}"
export PATH="$(pwd)/squashfs-root/usr/bin:$PATH"
pip install --upgrade pip
pip install -q -r Python/requirements.txt
mv squashfs-root/usr appdir/usr
mv squashfs-root/opt appdir/opt
rm -rf squashfs-root

# Fix RPATH on QtWebEngineProcess and copy missing resources
patchelf --set-rpath '$ORIGIN/../lib' appdir/libexec/QtWebEngineProcess

# Get Qt resources directory from qmake
QT_INSTALL_PREFIX=$(qmake -query QT_INSTALL_PREFIX 2>/dev/null || echo "")
if [ -n "$QT_INSTALL_PREFIX" ] && [ -d "${QT_INSTALL_PREFIX}/resources" ]; then
    cp -r "${QT_INSTALL_PREFIX}/resources" appdir/
else
    echo "Warning: Could not find Qt resources directory"
fi

# Generate AppImage
wget --no-verbose "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
chmod a+x appimagetool-x86_64.AppImage
./appimagetool-x86_64.AppImage appdir

### Cleanup
rm linuxdeployqt-continuous-x86_64.AppImage
rm appimagetool-x86_64.AppImage
rm -rf appdir

if [ ! -x ./GoldenCheetah*.AppImage ]; then
    echo "AppImage not generated, check the errors"
    exit 1
fi

echo "Renaming AppImage file to version number ready for deploy"
mv GoldenCheetah*.AppImage ../GoldenCheetah_v3.8_x64.AppImage

exit

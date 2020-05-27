#!/bin/bash
set -ev
export PATH=/opt/qt514/bin:$PATH
export LD_LIBRARY_PATH=/opt/qt514/lib/x86_64-linux-gnu:/opt/qt514/lib:$LD_LIBRARY_PATH

### This script should be run from GoldenCheetah src directory after build
cd src
if [ ! -x ./GoldenCheetah ]
then echo "Build GoldenCheetah and execute from distribution src"; exit 1
fi

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

### Add OpenSSL 1.1 libs (to make it easier for Xenial users)
mkdir appdir/lib
cp /usr/local/lib/libssl.so.1.1 appdir/lib
cp /usr/local/lib/libcrypto.so.1.1 appdir/lib

### Add vlc
cp -r /usr/lib/vlc appdir/lib/vlc

### Download current version of linuxdeployqt
wget --no-verbose -c https://github.com/probonopd/linuxdeployqt/releases/download/6/linuxdeployqt-6-x86_64.AppImage
chmod a+x linuxdeployqt-6-x86_64.AppImage

### Deploy to appdir
./linuxdeployqt-6-x86_64.AppImage appdir/GoldenCheetah -verbose=2 -bundle-non-qt-libs -exclude-libs=libqsqlmysql,libqsqlpsql,libnss3,libnssutil3,libxcb-dri3.so.0

# Add Python and core modules
wget https://github.com/niess/python-appimage/releases/download/python3.7/python3.7.7-cp37-cp37m-manylinux1_x86_64.AppImage
chmod +x python3.7.7-cp37-cp37m-manylinux1_x86_64.AppImage
./python3.7.7-cp37-cp37m-manylinux1_x86_64.AppImage --appimage-extract
rm -f python3.7.7-cp37-cp37m-manylinux1_x86_64.AppImage
export PATH="$(pwd)/squashfs-root/usr/bin:$PATH"
pip install --upgrade pip
pip install -r Python/requirements.txt
mv squashfs-root/usr appdir/usr
mv squashfs-root/opt appdir/opt
rm -rf squashfs-root

# Generate AppImage
wget "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
chmod a+x appimagetool-x86_64.AppImage
./appimagetool-x86_64.AppImage appdir

### Cleanup
rm linuxdeployqt-6-x86_64.AppImage
rm appimagetool-x86_64.AppImage
rm -rf appdir

if [ ! -x ./GoldenCheetah*.AppImage ]
then echo "AppImage not generated, check the errors"; exit 1
fi

echo "Renaming AppImage file to branch and build number ready for deploy"
export FINAL_NAME=GoldenCheetah_v3.6-DEV_x64.AppImage
mv GoldenCheetah*.AppImage $FINAL_NAME
ls -l $FINAL_NAME

### Minimum Test
./$FINAL_NAME --version 2>GCversionLinux.txt
git log -1 >> GCversionLinux.txt
echo "SHA256 hash of $FINAL_NAME:" >> GCversionLinux.txt
shasum -a 256 $FINAL_NAME | cut -f 1 -d ' '  >> GCversionLinux.txt
cat GCversionLinux.txt

### upload for testing
if [[ $TRAVIS_PULL_REQUEST == "false" && $TRAVIS_COMMIT_MESSAGE == *"[publish binaries]"* ]]; then
aws s3 rm s3://goldencheetah-binaries/Linux --recursive # keep only the last one
aws s3 cp --acl public-read $FINAL_NAME s3://goldencheetah-binaries/Linux/$FINAL_NAME
aws s3 cp --acl public-read GCversionLinux.txt s3://goldencheetah-binaries/Linux/GCversionLinux.txt
else
curl --max-time 300 --upload-file $FINAL_NAME https://transfer.sh/$FINAL_NAME
fi

cd ${TRAVIS_BUILD_DIR}
exit

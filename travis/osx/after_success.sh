#!/bin/bash
set -ev
cd src
echo "Checking GoldenCheetah.app can execute"
GoldenCheetah.app/Contents/MacOS/GoldenCheetah --help
echo "About to create dmg file and fix up"
# This is a hack to include libicudata.64.dylib, not handled by macdployqt[fix]
mkdir GoldenCheetah.app/Contents/Frameworks
cp /usr/local/opt/icu4c/lib/libicudata.64.dylib GoldenCheetah.app/Contents/Frameworks
# Copy python framework and change the path in binary
cp -R /usr/local/opt/python/Frameworks/Python.framework GoldenCheetah.app/Contents/Frameworks
install_name_tool -change `otool -L GoldenCheetah.app/Contents/MacOS/GoldenCheetah | awk '/python/ {print $1; gsub("/usr/local/opt/python", "@executable_path/..", $1); print $1}'` GoldenCheetah.app/Contents/MacOS/GoldenCheetah
# Fix QtWebEngineProcess due to bug in macdployqt from homebrew
/usr/local/opt/qt5/bin/macdeployqt GoldenCheetah.app -verbose=2 -executable=GoldenCheetah.app/Contents/MacOS/GoldenCheetah
pushd GoldenCheetah.app/Contents/Frameworks/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app/Contents/MacOS
for LIB in QtGui QtCore QtWebEngineCore QtQuick QtWebChannel QtNetwork QtPositioning
do
    OLD_PATH=`otool -L QtWebEngineProcess | grep ${LIB} | cut -f 1 -d ' '`
    NEW_PATH="@loader_path/../../../../../../../${LIB}.framework/${LIB}"
    echo ${OLD_PATH} ${NEW_PATH}
    install_name_tool -change ${OLD_PATH} ${NEW_PATH} QtWebEngineProcess
done
popd
# Deploy and generate dmg
/usr/local/opt/qt5/bin/macdeployqt GoldenCheetah.app -verbose=2 -fs=hfs+ -dmg
python ../travis/macdeployqtfix.py GoldenCheetah.app /usr/local/opt/qt5
echo "Renaming dmg file to branch and build number ready for deploy"
mv GoldenCheetah.dmg $FINAL_NAME
ls -l $FINAL_NAME
echo "Mounting dmg file and testing it can execute"
hdiutil mount $FINAL_NAME
cd /Volumes/GoldenCheetah
GoldenCheetah.app/Contents/MacOS/GoldenCheetah --help
echo "Uploading for user tests"
curl --upload-file $TRAVIS_BUILD_DIR/src/$FINAL_NAME https://transfer.sh/$FINAL_NAME
echo "Make sure we are back in the Travis build directory"
cd $TRAVIS_BUILD_DIR
exit

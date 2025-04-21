#!/bin/bash
set -ev
cd src

echo "About to create dmg file and fix up"
mkdir GoldenCheetah.app/Contents/Frameworks

# Add VLC dylibs and plugins
cp ../VLC/lib/libvlc.dylib ../VLC/lib/libvlccore.dylib GoldenCheetah.app/Contents/Frameworks
cp -R ../VLC/plugins GoldenCheetah.app/Contents/Frameworks

# This is a hack to include libicudata.*.dylib, not handled by macdployqt[fix]
cp /usr/local/opt/icu4c/lib/libicudata.*.dylib GoldenCheetah.app/Contents/Frameworks

# Copy python framework and change permissions to fix paths
cp -R /Library/Frameworks/Python.framework GoldenCheetah.app/Contents/Frameworks
chmod -R +w GoldenCheetah.app/Contents/Frameworks
# Update deployed Python framework path
install_name_tool -id @executable_path/../Frameworks/Python.framework/Versions/3.7/Python ./GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/3.7/Python
# Update GoldenCheetah binary to reference deployed lib
install_name_tool -change /Library/Frameworks/Python.framework/Versions/3.7/Python @executable_path/../Frameworks/Python.framework/Versions/3.7/Python ./GoldenCheetah.app/Contents/MacOS/GoldenCheetah
# Update Python binary to reference deployed lib instead of the Cellar one
OLD_PATH=`otool -L GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/3.7/bin/python3.7 | grep "Library" | cut -f 1 -d ' '`
echo $OLD_PATH
install_name_tool -change $OLD_PATH "@executable_path/../Python" GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/3.7/bin/python3.7
install_name_tool -change $OLD_PATH "@executable_path/../../../../Python" GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/3.7/Resources/Python.app/Contents/MacOS/Python
# Add mandatory Python dependencies
rm -r GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/3.7/lib/python3.7/site-packages
cp -R ../site-packages GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/3.7/lib/python3.7

# Initial deployment using macdeployqt
macdeployqt GoldenCheetah.app -verbose=2 -executable=GoldenCheetah.app/Contents/MacOS/GoldenCheetah

# Fix QtWebEngineProcess due to bug in macdployqt from homebrew
if [ ! -f GoldenCheetah.app/Contents/Frameworks/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess ]; then
    cp -Rafv /usr/local/Cellar/qt/5.15.?/lib/QtWebEngineCore.framework/Versions/5/Helpers/QtWebEngineProcess.app/Contents GoldenCheetah.app/Contents/Frameworks/QtWebEngineCore.framework/Versions/5/Helpers/QtWebEngineProcess.app
fi

pushd GoldenCheetah.app/Contents/Frameworks/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app/Contents/MacOS
for LIB in QtGui QtCore QtWebEngineCore QtQuick QtWebChannel QtNetwork QtPositioning QtQmlModels QtQml
do
    OLD_PATH=`otool -L QtWebEngineProcess | grep ${LIB}.framework | cut -f 1 -d ' '`
    NEW_PATH="@loader_path/../../../../../../../${LIB}.framework/${LIB}"
    echo ${OLD_PATH} ${NEW_PATH}
    install_name_tool -change ${OLD_PATH} ${NEW_PATH} QtWebEngineProcess
done
popd

# Final deployment to generate dmg (may take longer than 10' without output)
macdeployqt GoldenCheetah.app -verbose=2 -fs=hfs+ -dmg

echo "Renaming dmg file to branch and build number ready for deploy"
mv GoldenCheetah.dmg ../GoldenCheetah_v3.7_x64.dmg

exit

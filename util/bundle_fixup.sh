#!/bin/sh

if [ -d GoldenCheetah.app ]
then
   echo "Fixing app bundle: GoldenCheetah.app"
else
   echo "Make sure you run in build directory containing GoldenCheetah.app"
   exit
fi

echo "REBUILDING CLEAN BUNDLE"
rm -rf GoldenCheetah.app
make

echo "DEPLOYING STANDARD LIBS AND FRAMEWORKS INTO BUNDLE"
/Users/markliversedge/Qt510/5.9.4/clang_64/bin/macdeployqt GoldenCheetah.app GoldenCheeth.app

echo "COPY PYTHON FRAMEWORK INTO BUNDLE"
cp -R /Library/Frameworks/Python.framework ./GoldenCheetah.app/Contents/Frameworks

echo "UPDATE DEPLOYED PYTHON FRAMEWORK PATH"
sudo install_name_tool -id @executable_path/../Frameworks/Python.framework/Versions/3.6/Python ./GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/3.6/Python

echo "UPDATE GOLDENCHEETAH BINARY TO REFERENCE DEPLOYED LIB"
install_name_tool -change /Library/Frameworks/Python.framework/Versions/3.6/Python @executable_path/../Frameworks/Python.framework/Versions/3.6/Python ./GoldenCheetah.app/Contents/MacOS/GoldenCheetah 

echo "OTOOL OUTPUT FOR BINARY:"
otool -L ./GoldenCheetah.app/Contents/MacOS/GoldenCheetah

echo "MOVE TO DESKTOP"
mkdir ~/Desktop/GoldenCheetah
mv GoldenCheetah.app ~/Desktop/GoldenCheetah
hdiutil create -fs JHFS+ ~/Desktop/GoldenCheeetah.dmg -srcfolder ~/Desktop/GoldenCheetah -ov -format UDBZ

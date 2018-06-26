#!/bin/bash

set -x
# Do not exit. rvm overrides cd and returns !=0
# set -e

# try to get rid of the rvm debug logs
unalias -a
unset -f rvm_debug
unset -f cd
unset -f pushd
unset -f popd

cd src
echo "Checking GoldenCheetah.app can execute"
GoldenCheetah.app/Contents/MacOS/GoldenCheetah --help
echo "About to create dmg file and fix up"
/usr/local/opt/$QT_PATH/bin/macdeployqt GoldenCheetah.app -verbose=2 -dmg
python ../travis/macdeployqtfix.py GoldenCheetah.app /usr/local/opt/$QT_PATH
echo "Cleaning up installed QT libraries from $QT"
brew remove $QT
echo "Renaming dmg file to branch and build number ready for deploy"
export FINAL_NAME=dev-prerelease-branch-master-build-${TRAVIS_BUILD_NUMBER}.dmg
mv GoldenCheetah.dmg $FINAL_NAME
ls -l $FINAL_NAME
echo "Mounting dmg file and testing it can execute"
hdiutil mount $FINAL_NAME
cd /Volumes/GoldenCheetah
GoldenCheetah.app/Contents/MacOS/GoldenCheetah --help
echo "Make sure we are back in the Travis build directory"
cd $TRAVIS_BUILD_DIR

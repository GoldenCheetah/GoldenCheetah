#!/bin/bash
set -ev
cd src

PYTHON_VER=${MACOS_PYTHON_VERSION}
# Homebrew install location
BREW_PYTHON_FRAMEWORK="/usr/local/opt/python@${PYTHON_VER}/Frameworks/Python.framework"

echo "About to create dmg file and fix up"
mkdir -p GoldenCheetah.app/Contents/Frameworks

# This is a hack to include libicudata.*.dylib, not handled by macdployqt[fix]
cp /usr/local/opt/icu4c/lib/libicudata.*.dylib GoldenCheetah.app/Contents/Frameworks

# Copy python framework and change permissions to fix paths
echo "Copying Python Framework from ${BREW_PYTHON_FRAMEWORK}"
cp -R "${BREW_PYTHON_FRAMEWORK}" GoldenCheetah.app/Contents/Frameworks
chmod -R +w GoldenCheetah.app/Contents/Frameworks

# Update deployed Python framework path
# Change the ID of the library itself so it knows it lives in the app now
install_name_tool -id @executable_path/../Frameworks/Python.framework/Versions/${PYTHON_VER}/Python ./GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VER}/Python

# Update GoldenCheetah binary to reference deployed lib
# We replace the absolute path to the brew framework with the relative path inside the bundle
# Note: we attempt to replace the standard brew path. 
install_name_tool -change "${BREW_PYTHON_FRAMEWORK}/Versions/${PYTHON_VER}/Python" @executable_path/../Frameworks/Python.framework/Versions/${PYTHON_VER}/Python ./GoldenCheetah.app/Contents/MacOS/GoldenCheetah

# Update Python binary to reference deployed lib instead of the Cellar one
# This finds the library dependency of the python binary itself and points it to the bundled framework
PYTHON_BIN="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VER}/bin/python${PYTHON_VER}"
if [ -f "$PYTHON_BIN" ]; then
    OLD_PATH=`otool -L "$PYTHON_BIN" | grep "Python.framework" | grep -v executable_path | cut -f 1 -d ' ' | head -n 1`
    if [ ! -z "$OLD_PATH" ]; then
        echo "Updating python binary dependency from $OLD_PATH"
        install_name_tool -change "$OLD_PATH" "@executable_path/../Python" "$PYTHON_BIN"
    fi
else
    echo "Warning: Python binary not found at $PYTHON_BIN"
fi

# Same for the Python app stub if it exists
PYTHON_APP_BIN="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VER}/Resources/Python.app/Contents/MacOS/Python"
if [ -f "$PYTHON_APP_BIN" ]; then
     OLD_PATH_APP=`otool -L "$PYTHON_APP_BIN" | grep "Python.framework" | grep -v executable_path | cut -f 1 -d ' ' | head -n 1`
     if [ ! -z "$OLD_PATH_APP" ]; then
        install_name_tool -change "$OLD_PATH_APP" "@executable_path/../../../../Python" "$PYTHON_APP_BIN"
     fi
fi

# Add mandatory Python dependencies
SITE_PACKAGES="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VER}/lib/python${PYTHON_VER}/site-packages"
if [ -d "$SITE_PACKAGES" ]; then
    rm -r "$SITE_PACKAGES"
fi
# Ensure parent dir exists
mkdir -p "GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VER}/lib/python${PYTHON_VER}"
cp -R ../site-packages "$SITE_PACKAGES"

# Deployment using macdeployqt
macdeployqt GoldenCheetah.app -verbose=2 -executable=GoldenCheetah.app/Contents/MacOS/GoldenCheetah -fs=hfs+ -dmg

echo "Renaming dmg file to branch and build number ready for deploy"
mv GoldenCheetah.dmg ../GoldenCheetah_v3.7_x64.dmg

exit

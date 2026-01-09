#!/bin/bash
set -ev
cd src

PYTHON_VER=${MACOS_PYTHON_VERSION}
# Ensure we use the correct Python binary
export PATH="/usr/local/opt/python@${PYTHON_VER}/bin:$PATH"

# Homebrew install location
BREW_PYTHON_FRAMEWORK="/usr/local/opt/python@${PYTHON_VER}/Frameworks/Python.framework"

echo "About to create dmg file and fix up"
mkdir -p GoldenCheetah.app/Contents/Frameworks

# This is a hack to include libicudata.*.dylib, not handled by macdployqt[fix]
cp /usr/local/opt/icu4c/lib/libicudata.*.dylib GoldenCheetah.app/Contents/Frameworks

# Copy python framework and change permissions to fix paths
# Note: The framework already contains site-packages with numpy, sip, etc.
# because appveyor.yml runs "pip install -r requirements.txt" which installs
# packages into the brew Python's framework site-packages.
echo "Copying Python Framework from ${BREW_PYTHON_FRAMEWORK}"
# Remove any old attempts to avoid link confusion
rm -rf GoldenCheetah.app/Contents/Frameworks/Python.framework
rsync -axL "${BREW_PYTHON_FRAMEWORK}/" "GoldenCheetah.app/Contents/Frameworks/Python.framework/"

# This ensures every level of the path is a real directory
mkdir -p "GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VER}/lib/python${PYTHON_VER}"

chmod -R +w GoldenCheetah.app/Contents/Frameworks

# Locate the actual site-packages where pip installed dependencies (e.g. numpy)
# We rely on numpy being installed to find the true site-packages location
SITE_PACKAGES_SRC=$( /usr/local/opt/python@${PYTHON_VER}/bin/python${PYTHON_VER} -c "import numpy; import os; print(os.path.dirname(os.path.dirname(numpy.__file__)))" )
echo "Found source site-packages at: $SITE_PACKAGES_SRC"

if [ -z "$SITE_PACKAGES_SRC" ]; then
    echo "ERROR: Failed to locate site-packages using numpy."
    echo "Python command returned an empty string."
    echo "Aborting build to prevent rsync from copying the root filesystem."
    exit 1
fi

# Verify Python framework was copied correctly
PYTHON_LIB_DIR="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VER}/lib/python${PYTHON_VER}"
if [ ! -d "$PYTHON_LIB_DIR" ]; then
    echo "ERROR: Python framework lib directory not found at $PYTHON_LIB_DIR"
    echo "Contents of Frameworks directory:"
    ls -laR GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/ || true
    exit 1
fi


# Copy pip-installed packages from the actual install location
# Homebrew Python installs packages to /usr/local/lib, not into the framework
# Define the internal lib path clearly
INTERNAL_PYTHON_DIR="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VER}/lib/python${PYTHON_VER}"

echo "Merging pip-installed packages into bundle..."
# Create the parent path explicitly, avoiding symlink traversal issues
mkdir -p "$INTERNAL_PYTHON_DIR"

# Now create site-packages specifically
SITE_PACKAGES="${INTERNAL_PYTHON_DIR}/site-packages"
mkdir -p "$SITE_PACKAGES"

# Copy from the source found via the specific python version
rsync -ax --ignore-existing "$SITE_PACKAGES_SRC/" "$SITE_PACKAGES/"

# Update deployed Python framework path
# Change the ID of the library itself so it knows it lives in the app now
install_name_tool -id @executable_path/../Frameworks/Python.framework/Versions/${PYTHON_VER}/Python ./GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VER}/Python

# Update GoldenCheetah binary to reference deployed lib
# We replace the absolute path to the brew framework with the relative path inside the bundle
install_name_tool -change "${BREW_PYTHON_FRAMEWORK}/Versions/${PYTHON_VER}/Python" @executable_path/../Frameworks/Python.framework/Versions/${PYTHON_VER}/Python ./GoldenCheetah.app/Contents/MacOS/GoldenCheetah

# Update Python binary to reference deployed lib instead of the Cellar one
PYTHON_BIN="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VER}/bin/python${PYTHON_VER}"
if [ -f "$PYTHON_BIN" ]; then
    OLD_PATH=$(otool -L "$PYTHON_BIN" | grep "Python.framework" | grep -v executable_path | awk '{print $1}' | head -n 1)
    if [ -n "$OLD_PATH" ]; then
        echo "Updating python binary dependency from $OLD_PATH"
        install_name_tool -change "$OLD_PATH" "@executable_path/../Python" "$PYTHON_BIN"
    fi
else
    echo "Warning: Python binary not found at $PYTHON_BIN"
fi

# Same for the Python app stub if it exists
PYTHON_APP_BIN="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VER}/Resources/Python.app/Contents/MacOS/Python"
if [ -f "$PYTHON_APP_BIN" ]; then
    OLD_PATH_APP=$(otool -L "$PYTHON_APP_BIN" | grep "Python.framework" | grep -v executable_path | awk '{print $1}' | head -n 1)
    if [ -n "$OLD_PATH_APP" ]; then
        install_name_tool -change "$OLD_PATH_APP" "@executable_path/../../../../Python" "$PYTHON_APP_BIN"
    fi
fi

# Deployment using macdeployqt
macdeployqt GoldenCheetah.app -verbose=2 -executable=GoldenCheetah.app/Contents/MacOS/GoldenCheetah -fs=hfs+ -dmg

echo "Renaming dmg file to branch and build number ready for deploy"
mv GoldenCheetah.dmg ../GoldenCheetah_v3.8_x64.dmg

exit

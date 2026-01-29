#!/bin/bash
set -ev
cd src

# Ensure we use the correct Python binary
PYTHON_VER=${PYTHON_VERSION}
# Homebrew install location
BREW_PYTHON_ROOT=`brew --prefix python@${PYTHON_VER}`
export PATH="${BREW_PYTHON_ROOT}/bin:$PATH"
BREW_PYTHON_FRAMEWORK="${BREW_PYTHON_ROOT}/Frameworks/Python.framework"

echo "About to create dmg file and fix up"
mkdir -p GoldenCheetah.app/Contents/Frameworks

# This is a hack to include libicudata.*.dylib, not handled by macdployqt[fix]
cp `brew --prefix icu4c`/lib/libicudata.*.dylib GoldenCheetah.app/Contents/Frameworks

# Copy python framework and change permissions to fix paths
# Note: The framework already contains site-packages with numpy, sip, etc.
# because install.sh runs "pip install -r requirements.txt" which installs
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
SITE_PACKAGES_SRC=$( ${BREW_PYTHON_ROOT}/bin/python3 -c "import numpy; import os; print(os.path.dirname(os.path.dirname(numpy.__file__)))" )
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

# Fix the Python Framework structure broken by rsync -L
# rsync -L turns symlinks into directories, confusing codesign. We restore the standard structure.
echo "Restoring standard Python Framework structure..."
pushd GoldenCheetah.app/Contents/Frameworks/Python.framework > /dev/null
rm -rf Headers Resources Python Versions/Current
ln -s "${PYTHON_VER}" Versions/Current
ln -s Versions/Current/Headers Headers
ln -s Versions/Current/Resources Resources
ln -s Versions/Current/Python Python
popd > /dev/null

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
# Update GoldenCheetah binary to reference deployed lib
# We replace the absolute path to the brew framework with the relative path inside the bundle
GC_BIN="./GoldenCheetah.app/Contents/MacOS/GoldenCheetah"
OLD_GC_PATH=$(otool -L "$GC_BIN" | grep "Python.framework" | grep -v executable_path | awk '{print $1}' | head -n 1)
if [ -n "$OLD_GC_PATH" ]; then
    echo "Updating GoldenCheetah binary dependency from $OLD_GC_PATH"
    install_name_tool -change "$OLD_GC_PATH" "@executable_path/../Frameworks/Python.framework/Versions/${PYTHON_VER}/Python" "$GC_BIN"
else
    echo "GoldenCheetah binary already uses relative path or Python framework not found."
fi

# Update Python binary to reference deployed lib instead of the Cellar one
PYTHON_BIN="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VER}/bin/python${PYTHON_VER}"
if [ -f "$PYTHON_BIN" ]; then
    echo "Debugging dependencies for $PYTHON_BIN"
    otool -L "$PYTHON_BIN"
    # Iterate over all dependencies that look like Python and are absolute paths (not already @executable_path)
    otool -L "$PYTHON_BIN" | grep "Python" | grep "/" | grep -v "@executable_path" | awk '{print $1}' | while read OLD_PATH; do
        echo "Updating python binary dependency from $OLD_PATH"
        install_name_tool -change "$OLD_PATH" "@executable_path/../Python" "$PYTHON_BIN"
    done
else
    echo "Warning: Python binary not found at $PYTHON_BIN"
fi

# Same for the Python app stub if it exists
PYTHON_APP_BIN="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VER}/Resources/Python.app/Contents/MacOS/Python"
if [ -f "$PYTHON_APP_BIN" ]; then
    # Fix the app stub dependency too!
    otool -L "$PYTHON_APP_BIN" | grep "Python" | grep "/" | grep -v "@executable_path" | awk '{print $1}' | while read OLD_PATH_APP; do
        install_name_tool -change "$OLD_PATH_APP" "@executable_path/../../../../Python" "$PYTHON_APP_BIN"
    done
fi

# Fix libpython dependencies inside the framework to point to the internal framework binary
# This prevents macdeployqt from chasing external links and failing with path errors
find GoldenCheetah.app/Contents/Frameworks/Python.framework -name "libpython*.dylib" -type f | while read LIB; do
    echo "Fixing library dependency for: $LIB"
    OLD_LIB_PATH=$(otool -L "$LIB" | grep "Python.framework" | grep -v executable_path | awk '{print $1}' | head -n 1)
    if [ -n "$OLD_LIB_PATH" ]; then
        echo "  Changing $OLD_LIB_PATH to @executable_path/../Frameworks/Python.framework/Versions/${PYTHON_VER}/Python"
        install_name_tool -change "$OLD_LIB_PATH" "@executable_path/../Frameworks/Python.framework/Versions/${PYTHON_VER}/Python" "$LIB"
    fi
    # Also fix the ID of the dylib itself if needed (macdeployqt likes valid IDs)
    install_name_tool -id "@executable_path/../Frameworks/Python.framework/Versions/${PYTHON_VER}/lib/$(basename $LIB)" "$LIB"
done

# Fix missing QtDBus framework (required by QtGui but missed by macdeployqt)
# We copy it manually so it's present when we sign
echo "Manually copying QtDBus framework..."
cp -R "${QTDIR}/lib/QtDBus.framework" GoldenCheetah.app/Contents/Frameworks/

# Deployment using macdeployqt - prepare bundle only
macdeployqt GoldenCheetah.app -verbose=2 -executable=GoldenCheetah.app/Contents/MacOS/GoldenCheetah

echo "Resigning application bundle..."
# Explicitly sign the Python components first to fix invalid Ad-Hoc signatures caused by install_name_tool
echo "Forcing signature refresh on Python components..."
codesign --force --sign - --preserve-metadata=identifier,entitlements "GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VER}/Python"
codesign --force --sign - --preserve-metadata=identifier,entitlements "GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VER}/bin/python${PYTHON_VER}"
if [ -f "$PYTHON_APP_BIN" ]; then
    codesign --force --sign - --preserve-metadata=identifier,entitlements "$PYTHON_APP_BIN"
fi

# - sign with ad-hoc identity check, this is free and works for local dev
# - force rewrite of existing signatures (invalidated by install_name_tool)
# - deep sign frameworks and plugins
codesign --force --deep --sign - GoldenCheetah.app

echo "Creating dmg file..."
# Manually create DMG since we removed -dmg from macdeployqt
hdiutil create -volname GoldenCheetah -srcfolder GoldenCheetah.app -ov -format UDZO GoldenCheetah.dmg

echo "Renaming dmg file to branch and build number ready for deploy"
mv GoldenCheetah.dmg ../GoldenCheetah_v3.8_x64.dmg

#!/bin/bash
set -ev
cd src
export PIP_BREAK_SYSTEM_PACKAGES=1

# Homebrew install location
BREW_PYTHON_ROOT=`brew --prefix python@${PYTHON_VERSION}`
export PATH="${BREW_PYTHON_ROOT}/bin:$PATH"
BREW_PYTHON_FRAMEWORK="${BREW_PYTHON_ROOT}/Frameworks/Python.framework"

echo "About to create dmg file and fix up"
mkdir -p GoldenCheetah.app/Contents/Frameworks

# This is a hack to include libicudata.*.dylib, not handled by macdployqt[fix]
cp `brew --prefix icu4c`/lib/libicudata.*.dylib GoldenCheetah.app/Contents/Frameworks

# Copy python framework and change permissions to fix paths
echo "Copying Python Framework from ${BREW_PYTHON_FRAMEWORK}"
# Remove any old attempts to avoid link confusion
rm -rf GoldenCheetah.app/Contents/Frameworks/Python.framework
rsync -axL "${BREW_PYTHON_FRAMEWORK}/" "GoldenCheetah.app/Contents/Frameworks/Python.framework/"

# Fix the Python Framework structure broken by rsync -L
# rsync -L turns symlinks into directories/files, confusing codesign. We restore the standard structure.
echo "Restoring standard Python Framework structure..."
pushd GoldenCheetah.app/Contents/Frameworks/Python.framework > /dev/null
rm -rf Headers Resources Python Versions/Current
ln -s "${PYTHON_VERSION}" Versions/Current
ln -s Versions/Current/Headers Headers
ln -s Versions/Current/Resources Resources
ln -s Versions/Current/Python Python
popd > /dev/null

# This ensures every level of the path is a real directory
mkdir -p "GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/lib/python${PYTHON_VERSION}"

chmod -R +w GoldenCheetah.app/Contents/Frameworks

PYTHON_BIN="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/bin/python${PYTHON_VERSION}"

echo "Installing requirements into bundle..."
if [ -f "$PYTHON_BIN" ]; then
    # Fix RPATH early
    install_name_tool -add_rpath "@executable_path/../.." "$PYTHON_BIN" || true

    # Re-sign the binary immediately because install_name_tool invalidated it
    codesign --force --sign - "$PYTHON_BIN"

    echo "Running pip install using bundled python: $PYTHON_BIN"
    # Ensure modern build tools
    "$PYTHON_BIN" -m pip install --upgrade pip setuptools wheel

    # Install all requirements at once
    # Calculate target site-packages
    # Note: AppVeyor script might define variables differently, but structure is standard.
    # 'GoldenCheetah.app' is in the current directory (src)
    SITE_PACKAGES="$(pwd)/GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/Current/lib/python3.11/site-packages"
    if [ ! -d "$SITE_PACKAGES" ]; then
         SITE_PACKAGES=$(find "$(pwd)/GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/Current/lib" -name "python3.*" -type d | head -n 1)/site-packages
    fi
    echo "Installing Python packages to bundle target: $SITE_PACKAGES"

    "$PYTHON_BIN" -m pip install --target "$SITE_PACKAGES" --ignore-installed --break-system-packages --no-cache-dir --only-binary :all: -r ../src/Python/requirements.txt
else
    echo "ERROR: Bundled python binary not found at $PYTHON_BIN"
    exit 1
fi

SITE_PACKAGES_SRC=$( "$PYTHON_BIN" -c "import numpy; import os; print(os.path.dirname(os.path.dirname(numpy.__file__)))" )
echo "Verified Site Packages at: $SITE_PACKAGES_SRC"

# Remove direct_url.json metadata which may contain absolute paths to the build machine
find "$SITE_PACKAGES_SRC" -name "direct_url.json" -delete

# Update deployed Python framework path
# Change the ID of the library itself so it knows it lives in the app now
install_name_tool -id @executable_path/../Frameworks/Python.framework/Versions/${PYTHON_VERSION}/Python ./GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/Python

# Update GoldenCheetah binary to reference deployed lib
# We replace the absolute path to the brew framework with the relative path inside the bundle
# Update GoldenCheetah binary to reference deployed lib
# We replace the absolute path to the brew framework with the relative path inside the bundle
GC_BIN="./GoldenCheetah.app/Contents/MacOS/GoldenCheetah"
OLD_GC_PATH=$(otool -L "$GC_BIN" | grep "Python.framework" | grep -v executable_path | awk '{print $1}' | head -n 1)
if [ -n "$OLD_GC_PATH" ]; then
    echo "Updating GoldenCheetah binary dependency from $OLD_GC_PATH"
    install_name_tool -change "$OLD_GC_PATH" "@executable_path/../Frameworks/Python.framework/Versions/${PYTHON_VERSION}/Python" "$GC_BIN"
else
    echo "GoldenCheetah binary already uses relative path or Python framework not found."
fi

# Update Python binary to reference deployed lib instead of the Cellar one
PYTHON_BIN="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/bin/python${PYTHON_VERSION}"
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
PYTHON_APP_BIN="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/Resources/Python.app/Contents/MacOS/Python"
if [ -f "$PYTHON_APP_BIN" ]; then
    # Fix the app stub dependency too!
    otool -L "$PYTHON_APP_BIN" | grep "Python" | grep "/" | grep -v "@executable_path" | awk '{print $1}' | while read OLD_PATH_APP; do
        install_name_tool -change "$OLD_PATH_APP" "@executable_path/../../../../Python" "$PYTHON_APP_BIN"
    done
fi

# Fix pip binaries to be relocatable (replace shebangs)
echo "Fixing pip binaries to be relocatable..."
PYTHON_BIN_DIR="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/bin"
if [ -d "$PYTHON_BIN_DIR" ]; then
    for PIP_BIN in pip pip3 pip${PYTHON_VERSION}; do
        if [ -f "$PYTHON_BIN_DIR/$PIP_BIN" ]; then
            echo "Rewriting shebang for $PIP_BIN"
            rm "$PYTHON_BIN_DIR/$PIP_BIN"
            cat > "$PYTHON_BIN_DIR/$PIP_BIN" <<EOF
#!/bin/sh
exec "\$(dirname "\$0")/python${PYTHON_VERSION}" -m pip "\$@"
EOF
            chmod +x "$PYTHON_BIN_DIR/$PIP_BIN"
        fi
    done
fi

# Patch _sysconfigdata to remove absolute paths from build machine
echo "Patching _sysconfigdata..."
SYSCONFIG_FILE=$(find "GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/lib/python${PYTHON_VERSION}" -name "_sysconfigdata_*.py" | head -n 1)
if [ -f "$SYSCONFIG_FILE" ]; then
    echo "Patching $SYSCONFIG_FILE to be relocatable..."
    # Use python to safely replace the string literal, avoiding sed quoting issues
    # BREW_PYTHON_ROOT is set at top of script
    python3 <<EOF
import sys
import os

filepath = "$SYSCONFIG_FILE"
prefix = "$BREW_PYTHON_ROOT"

if os.path.exists(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    # Ensure sys is imported
    if "import sys" not in content:
        content = "import sys\n" + content

    # Replace absolute prefix with f-string using sys.prefix
    # This preserves implicit string concatenation (e.g. f'...' '...')
    content = content.replace("'" + prefix, "f'{sys.prefix}")
    content = content.replace('"' + prefix, 'f"{sys.prefix}')

    with open(filepath, 'w') as f:
        f.write(content)
EOF
fi

# Fix libpython dependencies inside the framework to point to the internal framework binary
# This prevents macdeployqt from chasing external links and failing with path errors
find GoldenCheetah.app/Contents/Frameworks/Python.framework -name "libpython*.dylib" -type f | while read LIB; do
    echo "Fixing library dependency for: $LIB"
    OLD_LIB_PATH=$(otool -L "$LIB" | grep "Python.framework" | grep -v executable_path | awk '{print $1}' | head -n 1)
    if [ -n "$OLD_LIB_PATH" ]; then
        echo "  Changing $OLD_LIB_PATH to @executable_path/../Frameworks/Python.framework/Versions/${PYTHON_VERSION}/Python"
        install_name_tool -change "$OLD_LIB_PATH" "@executable_path/../Frameworks/Python.framework/Versions/${PYTHON_VERSION}/Python" "$LIB"
    fi
    # Also fix the ID of the dylib itself if needed (macdeployqt likes valid IDs)
    install_name_tool -id "@executable_path/../Frameworks/Python.framework/Versions/${PYTHON_VERSION}/lib/$(basename $LIB)" "$LIB"
done

# OpenSSL bundling for Python _ssl module
echo "Bundling OpenSSL libraries for Python..."
OPENSSL_PREFIX=$(brew --prefix openssl@3)
if [ -d "$OPENSSL_PREFIX" ]; then
    echo "Found OpenSSL at $OPENSSL_PREFIX"

    # Destination for OpenSSL libs (same level as Python framework usually, or inside it)
    # Putting them in Frameworks/ is standard
    DEST_FRAMEWORKS="GoldenCheetah.app/Contents/Frameworks"

    # Copy the dylibs
    cp "$OPENSSL_PREFIX/lib/libssl.3.dylib" "$DEST_FRAMEWORKS/"
    cp "$OPENSSL_PREFIX/lib/libcrypto.3.dylib" "$DEST_FRAMEWORKS/"

    # Make them writable for install_name_tool
    chmod +w "$DEST_FRAMEWORKS/libssl.3.dylib"
    chmod +w "$DEST_FRAMEWORKS/libcrypto.3.dylib"

    # Fix IDs of the copied libs
    # Fix IDs of the copied libs
    install_name_tool -id "@loader_path/libssl.3.dylib" "$DEST_FRAMEWORKS/libssl.3.dylib"
    install_name_tool -id "@loader_path/libcrypto.3.dylib" "$DEST_FRAMEWORKS/libcrypto.3.dylib"

    # Fix dependency of libssl on libcrypto - use @loader_path (they are in same dir)
    install_name_tool -change "$OPENSSL_PREFIX/lib/libcrypto.3.dylib" "@loader_path/libcrypto.3.dylib" "$DEST_FRAMEWORKS/libssl.3.dylib"

    # Now find the python extensions that need them (_ssl, _hashlib)
    # They are in lib-dynload
    DYNLOAD_DIR="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/lib/python${PYTHON_VERSION}/lib-dynload"

    if [ -d "$DYNLOAD_DIR" ]; then
        for EXT in _ssl _hashlib; do
            # Find the actual so file (e.g. _ssl.cpython-311-darwin.so)
            EXT_FILE=$(find "$DYNLOAD_DIR" -name "${EXT}.*.so" | head -n 1)

            if [ -n "$EXT_FILE" ]; then
                echo "Patching $EXT_FILE"
                # Update linkage to find libssl/libcrypto relative to _ssl.so
                # _ssl.so is in .../lib/python3.11/lib-dynload
                # libssl is in .../Frameworks
                # Path is @loader_path/../../../../../../libssl.3.dylib
                install_name_tool -change "$OPENSSL_PREFIX/lib/libssl.3.dylib" "@loader_path/../../../../../../libssl.3.dylib" "$EXT_FILE"
                install_name_tool -change "$OPENSSL_PREFIX/lib/libcrypto.3.dylib" "@loader_path/../../../../../../libcrypto.3.dylib" "$EXT_FILE"
            else
                echo "Warning: Could not find extension for $EXT in $DYNLOAD_DIR"
            fi
        done
        # Also patch hashlib
    else
        echo "Error: lib-dynload directory not found at $DYNLOAD_DIR"
        exit 1
    fi

else
    echo "Error: Could not find openssl@3 prefix"
    exit 1
fi

# Fix missing QtDBus framework (required by QtGui but missed by macdeployqt)
# We copy it manually so it's present when we sign
echo "Manually copying QtDBus framework..."
cp -R "${QTDIR}/lib/QtDBus.framework" GoldenCheetah.app/Contents/Frameworks/

# Deployment using macdeployqt - prepare bundle only
macdeployqt GoldenCheetah.app -verbose=2 -executable=GoldenCheetah.app/Contents/MacOS/GoldenCheetah

### MANUAL LEAK PATCHING ###
echo "Starting manual leak patching..."

# Helper: Fix the ID of a binary to be relative (@rpath)
fix_binary_id() {
    local BINARY="$1"
    local BINARY_ID=$(otool -D "$BINARY" | grep -v ":" | head -n 1)

    # Ensure writable
    chmod +w "$BINARY"

    # Check if ID is a system path (excluding /System/Library)
    if [[ "$BINARY_ID" == *"/opt/homebrew"* ]] || [[ "$BINARY_ID" == *"/usr/local"* ]] || [[ "$BINARY_ID" == *"/Users"* ]] || [[ "$BINARY_ID" == *"/Library/Frameworks"* ]]; then
        local NEW_ID=""
        if [[ "$BINARY" == *".framework"* ]]; then
            # Extract framework relative path: .../Foo.framework/Versions/A/Foo -> Foo.framework/Versions/A/Foo
            local REL_PATH=$(echo "$BINARY" | sed -E 's/.*\/([^\/]+\.framework.*)/\1/')
            NEW_ID="@rpath/$REL_PATH"
        else
            # Flat lib: libfoo.dylib
            local LIB_NAME=$(basename "$BINARY")
            NEW_ID="@rpath/$LIB_NAME"
        fi

        echo "  Fixing ID for $BINARY"
        echo "    Old: $BINARY_ID"
        echo "    New: $NEW_ID"
        install_name_tool -id "$NEW_ID" "$BINARY"
    fi
}

# Helper: Fix dependencies of a binary
fix_binary_deps() {
    local BINARY="$1"

    # Check deps
    otool -L "$BINARY" | grep -E "(/usr/local/|/opt/homebrew/|/Users/|/Library/Frameworks/)" | grep -v "/System/" | awk '{print $1}' | while read LEAK_PATH; do
        local DEST_REL=""
        if [[ "$LEAK_PATH" == *".framework"* ]]; then
             local REL_PATH=$(echo "$LEAK_PATH" | sed -E 's/.*\/([^\/]+\.framework.*)/\1/')
             DEST_REL="@rpath/$REL_PATH"
        else
             local LIB_NAME=$(basename "$LEAK_PATH")
             if [ -f "GoldenCheetah.app/Contents/Frameworks/$LIB_NAME" ]; then
                  DEST_REL="@rpath/$LIB_NAME"
             else
                  if [ -f "$LEAK_PATH" ]; then
                       echo "  Copying missing lib $LIB_NAME to bundle from $LEAK_PATH..."
                       cp "$LEAK_PATH" "GoldenCheetah.app/Contents/Frameworks/"
                       chmod +w "GoldenCheetah.app/Contents/Frameworks/$LIB_NAME"
                       # Recursively fix the new lib
                       fix_binary_id "GoldenCheetah.app/Contents/Frameworks/$LIB_NAME"
                       fix_binary_deps "GoldenCheetah.app/Contents/Frameworks/$LIB_NAME"
                       DEST_REL="@rpath/$LIB_NAME"
                  else
                       echo "  WARNING: Lib $LIB_NAME not found in bundle OR system ($LEAK_PATH)."
                  fi
             fi
        fi

        if [ -n "$DEST_REL" ]; then
             echo "  Relinking dep $LEAK_PATH -> $DEST_REL in $BINARY"
             install_name_tool -change "$LEAK_PATH" "$DEST_REL" "$BINARY"
        fi
    done
}


# 0. cleanup static archives
find GoldenCheetah.app/Contents/Frameworks -name "*.a" -delete

# 1. QtWebEngineProcess
QWEBVIEW_APP="GoldenCheetah.app/Contents/Frameworks/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess"
if [ -f "$QWEBVIEW_APP" ]; then
    echo "Patching QtWebEngineProcess..."
    install_name_tool -add_rpath "@executable_path/../../../../../../../" "$QWEBVIEW_APP" || true
    fix_binary_deps "$QWEBVIEW_APP"
fi

# 2. Python Binaries
echo "Patching Python binaries..."
# Manual RPATH fix for python binaries so they can find @rpath/Python.framework...
# In AppVeyor script, PYTHON_VERSION is used instead of PYTHON_FULL_VER
PYTHON_BIN="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/bin/python${PYTHON_VERSION}"
if [ -f "$PYTHON_BIN" ]; then
    install_name_tool -add_rpath "@executable_path/../.." "$PYTHON_BIN" || true
    fix_binary_deps "$PYTHON_BIN"
fi

PYTHON_APP_BIN="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/Resources/Python.app/Contents/MacOS/Python"
if [ -f "$PYTHON_APP_BIN" ]; then
     install_name_tool -add_rpath "@executable_path/../../../../../../.." "$PYTHON_APP_BIN" || true
     fix_binary_deps "$PYTHON_APP_BIN"
fi

# 3. Mass Scan
echo "Scanning entire bundle for other leaks..."
set +v
# Use user's improved find command
find GoldenCheetah.app/Contents/MacOS GoldenCheetah.app/Contents/Frameworks \( -name "GoldenCheetah" -o -name "*.dylib" -o -name "*.so" -o -perm +111 \) -type f | sort -u | while read BINARY; do
    if file "$BINARY" | grep -q "Mach-O"; then
        fix_binary_id "$BINARY"
        fix_binary_deps "$BINARY"
    fi
done
set -v

echo "Resigning application bundle..."
# Explicitly sign the Python components first to fix invalid Ad-Hoc signatures caused by install_name_tool
echo "Forcing signature refresh on Python components..."
codesign --force --sign - "GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/Python"
codesign --force --sign - "GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/bin/python${PYTHON_VERSION}"
if [ -f "$PYTHON_APP_BIN" ]; then
    codesign --force --sign - "$PYTHON_APP_BIN"
fi

# Explicitly resign all .so and .dylib files in the framework (e.g. in lib-dynload)
# codesign --deep on the app bundle often skips these or fails to resign them properly
echo "Resigning all dynamic libraries in Python framework AND Contents/Frameworks..."
find "GoldenCheetah.app/Contents/Frameworks" -type f \( -name "*.dylib" -o -name "*.so" \) -exec codesign --force --sign - {} \;

# Sign the nested Python.app if it exists (Inside-Out signing)
PYTHON_APP="GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/Resources/Python.app"
if [ -d "$PYTHON_APP" ]; then
    echo "Signing nested Python.app..."
    codesign --force --preserve-metadata=identifier,entitlements --sign - "$PYTHON_APP"
fi

# Sign the Python framework itself
# We verified structure earlier.
echo "Signing Python.framework..."
codesign --force --sign - "GoldenCheetah.app/Contents/Frameworks/Python.framework"

# - sign with ad-hoc identity check, this is free and works for local dev
# - force rewrite of existing signatures (invalidated by install_name_tool)
# - deep sign frameworks and plugins
echo "Signing final GoldenCheetah.app..."
codesign --force --deep --sign - GoldenCheetah.app

echo "Creating dmg file..."
# Manually create DMG since we removed -dmg from macdeployqt
hdiutil create -volname GoldenCheetah -srcfolder GoldenCheetah.app -ov -format UDZO GoldenCheetah.dmg

echo "Renaming dmg file to branch and build number ready for deploy"
mv GoldenCheetah.dmg ../GoldenCheetah_v3.8_x64.dmg

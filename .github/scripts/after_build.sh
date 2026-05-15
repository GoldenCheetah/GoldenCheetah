#!/usr/bin/env bash
# shellcheck enable=all
# shellcheck shell=bash
# shellcheck disable=SC2312

set -Eeuf -o pipefail

log() {
  printf '%s\n' "$*" >&2
}

err() {
  log "$*"
  exit 1
}

# Helper: Fix the ID of a binary to be relative (@rpath)
fix_binary_id() {
  local BINARY_ID BINARY
  BINARY=$1
  BINARY_ID=$(
    otool -D "${BINARY}" |
      awk '$1 !~ /:$/ { print; exit; }'
  )

  # Ensure writable
  chmod +w "${BINARY}"

  # Check if ID is a system path (excluding /System/Library)
  case "${BINARY_ID}" in
    "/opt/homebrew/"* | *"/Users/"* | *"/Library/Frameworks/"*)
      :
      ;;
    *)
      return
      ;;
  esac

  local NEW_ID
  if [[ "${BINARY}" == *".framework"* ]]; then
    # Extract framework relative path: .../Foo.framework/Versions/A/Foo -> Foo.framework/Versions/A/Foo
    local REL_PATH
    : "${BINARY%/*.framework/*}"
    REL_PATH=${BINARY#"${_}/"}
    NEW_ID=@rpath/${REL_PATH}
  else
    # Flat lib: libfoo.dylib
    local LIB_NAME
    LIB_NAME=${BINARY##*/}
    NEW_ID=@rpath/${LIB_NAME}
  fi

  log "  Fixing ID for ${BINARY}"
  log "    Old: ${BINARY_ID}"
  log "    New: ${NEW_ID}"
  install_name_tool -id "${NEW_ID}" "${BINARY}"
}

# Helper: Fix dependencies of a binary
fix_binary_deps() {
  local BINARY=$1

  # Check deps
  otool -L "${BINARY}" |
    awk \
      -v includes='/opt/homebrew/|opt/homebrew/|Users/|Library/Frameworks/' \
      -v excludes='/System/' \
      '
      $0 ~ includes && $0 !~ excludes { print $1 }
    ' |
    while read -r LEAK_PATH; do
      local REL_PATH DEST_REL LIB_NAME
      if [[ "${LEAK_PATH}" == *".framework"* ]]; then
        REL_PATH=$(sed -E 's/.*\/([^\/]+\.framework.*)/\1/' <<< "${LEAK_PATH}")
        DEST_REL=@rpath/${REL_PATH}
      else
        LIB_NAME=$(basename "${LEAK_PATH}")
        if [[ -f "GoldenCheetah.app/Contents/Frameworks/${LIB_NAME}" ]]; then
          DEST_REL=@rpath/${LIB_NAME}
        else
          if [[ -f "${LEAK_PATH}" ]]; then
            log "  Copying missing lib ${LIB_NAME} to bundle from ${LEAK_PATH}..."
            cp -R "${LEAK_PATH}" "GoldenCheetah.app/Contents/Frameworks/"
            chmod +w "GoldenCheetah.app/Contents/Frameworks/${LIB_NAME}"
            # Recursively fix the new lib
            fix_binary_id "GoldenCheetah.app/Contents/Frameworks/${LIB_NAME}"
            fix_binary_deps "GoldenCheetah.app/Contents/Frameworks/${LIB_NAME}"
            DEST_REL=@rpath/${LIB_NAME}
          else
            log "  WARNING: Lib ${LIB_NAME} not found in bundle OR system (${LEAK_PATH})."
          fi
        fi
      fi

      if [[ -n "${DEST_REL}" ]]; then
        log "  Relinking dep ${LEAK_PATH} -> ${DEST_REL} in ${BINARY}"
        install_name_tool -change "${LEAK_PATH}" "${DEST_REL}" "${BINARY}"
      fi
    done
}

main() {
  local PYTHON_VERSION=3.11
  pushd src

  log "About to create dmg file and fix up"
  mkdir -p GoldenCheetah.app/Contents/Frameworks

  # This is a hack to include libicudata.*.dylib, not handled by macdployqt[fix]
  find \
    "$(brew --prefix icu4c)/lib" \
    -maxdepth 1 \
    -name 'libicudata.*.dylib' \
    -exec cp {} GoldenCheetah.app/Contents/Frameworks \;

  # Copy python framework and change permissions to fix paths
  local BREW_PYTHON_ROOT
  BREW_PYTHON_ROOT=$(brew --prefix python@"${PYTHON_VERSION?}")
  local BREW_PYTHON_FRAMEWORK=${BREW_PYTHON_ROOT}/Frameworks/Python.framework
  log "Copying Python Framework from ${BREW_PYTHON_FRAMEWORK}"

  # Remove any old attempts to avoid link confusion
  rm -rf GoldenCheetah.app/Contents/Frameworks/Python.framework
  cp -R "${BREW_PYTHON_FRAMEWORK}" GoldenCheetah.app/Contents/Frameworks/

  local DST_PYTHON_FRAMEWORK=GoldenCheetah.app/Contents/Frameworks/Python.framework

  rm -f "${DST_PYTHON_FRAMEWORK}"/{Python,Resources,Headers,Versions/Current}
  ln -s "${PYTHON_VERSION}" \
    "${DST_PYTHON_FRAMEWORK}/Versions/Current"
  ln -s "Versions/Current/Python" \
    "${DST_PYTHON_FRAMEWORK}/Python"
  ln -s "Versions/Current/Resources" \
    "${DST_PYTHON_FRAMEWORK}/Resources"
  ln -s "Versions/Current/Headers" \
    "${DST_PYTHON_FRAMEWORK}/Headers"

  # site-packages is copied as a symlink to /opt/homebrew/lib/python3.11/site-packages
  # promote it to a real directory
  local SITE_PACKAGES=./GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/Current/lib/python${PYTHON_VERSION}/site-packages
  rm "${SITE_PACKAGES}"
  mkdir -p "${SITE_PACKAGES}"

  # Ensure modern build tools
  python"${PYTHON_VERSION}" -m pip install --upgrade pip setuptools wheel

  log "Installing Python packages to bundle target: ${SITE_PACKAGES}"
  python"${PYTHON_VERSION}" -m pip \
    install \
    --target "${SITE_PACKAGES}" \
    --only-binary :all: \
    -r ../src/Python/requirements.txt

  chmod -R +w GoldenCheetah.app/Contents/Frameworks

  local PYTHON_BIN=GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python${PYTHON_VERSION}

  # # Update deployed Python framework path
  # # Change the ID of the library itself so it knows it lives in the app now
  install_name_tool -id @executable_path/../Frameworks/Python.framework/Versions/Current/Python ./GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/Current/Python

  # Update GoldenCheetah binary to reference deployed lib
  # We replace the absolute path to the brew framework with the relative path inside the bundle
  local GC_BIN=GoldenCheetah.app/Contents/MacOS/GoldenCheetah
  local OLD_GC_PATH
  OLD_GC_PATH=$(
    otool -L "${GC_BIN}" |
      awk '/Python\.framework/ && !/executable_path/ { print $1; exit; }'
  )
  if [[ -n "${OLD_GC_PATH}" ]]; then
    log "Updating GoldenCheetah binary dependency from ${OLD_GC_PATH}"
    install_name_tool -change "${OLD_GC_PATH}" "@executable_path/../Frameworks/Python.framework/Versions/Current/Python" "${GC_BIN}"
  else
    log "GoldenCheetah binary already uses relative path or Python framework not found."
  fi

  # Update Python binary to reference deployed lib instead of the Cellar one
  otool -L "${PYTHON_BIN}" |
    awk '
      # trim the leading space from otool output (on all but first line)
      { sub(/^[[:space:]]/, "") }

      # first line prints the current binary and ends with colon; exclude
      $1 !~ /:$/ &&
        # include only files that look like Python
        /Python/ &&
        # include only absolute paths
        /^\// &&
        # exclude anything already set to use rpath
        $0 !~ /@executable_path/ \
        { print $1 }
      ' |
    while read -r OLD_PATH; do
      log "Updating python binary dependency from ${OLD_PATH}"
      install_name_tool -change "${OLD_PATH}" "@executable_path/../Python" "${PYTHON_BIN}"
    done

  # Same for the Python app stub if it exists
  local PYTHON_APP_BIN=GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/Current/Resources/Python.app/Contents/MacOS/Python
  otool -L "${PYTHON_BIN}" |
    awk '
      # trim the leading space from otool output (on all but first line)
      { sub(/^[[:space:]]/, "") }

      # first line prints the current binary and ends with colon; exclude
      $1 !~ /:$/ &&
        # include only files that look like Python
        /Python/ &&
        # include only absolute paths
        /^\// &&
        # exclude anything already set to use rpath
        $0 !~ /@executable_path/ \
        { print $1 }
      ' |
    while read -r OLD_PATH_APP; do
      install_name_tool -change "${OLD_PATH_APP}" "@executable_path/../../../../Python" "${PYTHON_APP_BIN}"
    done

  # Fix pip binaries to be relocatable (replace shebangs)
  log "Fixing pip binaries to be relocatable..."
  local PYTHON_BIN_DIR=GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/Current/bin
  local PIP_BIN=${PYTHON_BIN_DIR}/pip3
  log "Rewriting shebang for ${PIP_BIN}"
  rm "${PIP_BIN}"
  cat > "${PIP_BIN}" << EOF
#!/bin/sh
exec "\$(dirname "\$0")/python${PYTHON_VERSION}" -m pip "\$@"
EOF
  chmod +x "${PIP_BIN}"

  # Patch _sysconfigdata to remove absolute paths from build machine
  log "Patching _sysconfigdata..."
  local SYSCONFIG_FILE
  SYSCONFIG_FILE=$(
    find \
      "GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/Current/lib/python${PYTHON_VERSION}" \
      -name "_sysconfigdata_*.py" \
      -print -quit
  )
  log "Patching ${SYSCONFIG_FILE} to be relocatable..."
  # Use python to safely replace the string literal, avoiding sed quoting issues
  # BREW_PYTHON_ROOT is set at top of script
  python3 << EOF
import sys
import os

filepath = "${SYSCONFIG_FILE}"
prefix = "${BREW_PYTHON_ROOT}"

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

  # Fix libpython dependencies inside the framework to point to the internal framework binary
  # This prevents macdeployqt from chasing external links and failing with path errors
  find GoldenCheetah.app/Contents/Frameworks/Python.framework \
    -name "libpython*.dylib" \
    -type f |
    while read -r LIB; do
      log "Fixing library dependency for: ${LIB}"
      local OLD_LIB_PATH
      OLD_LIB_PATH=$(
        otool -L "${LIB}" |
          awk '/Python\.framework/ && !/executable_path/ { print $1; exit; }'
      )
      if [[ -n "${OLD_LIB_PATH}" ]]; then
        log "  Changing ${OLD_LIB_PATH} to @executable_path/../Frameworks/Python.framework/Versions/Current/Python"
        install_name_tool -change "${OLD_LIB_PATH}" "@executable_path/../Frameworks/Python.framework/Versions/Current/Python" "${LIB}"
      fi
      # Also fix the ID of the dylib itself if needed (macdeployqt likes valid IDs)
      install_name_tool -id "@executable_path/../Frameworks/Python.framework/Versions/Current/lib/$(basename "${LIB}")" "${LIB}"
    done

  # OpenSSL bundling for Python _ssl module
  log "Bundling OpenSSL libraries for Python..."
  local OPENSSL_PREFIX
  OPENSSL_PREFIX=$(brew --prefix openssl@3)
  log "Found OpenSSL at ${OPENSSL_PREFIX}"

  # Destination for OpenSSL libs (same level as Python framework usually, or inside it)
  # Putting them in Frameworks/ is standard
  local DEST_FRAMEWORKS
  DEST_FRAMEWORKS=GoldenCheetah.app/Contents/Frameworks

  # Copy the dylibs
  cp "${OPENSSL_PREFIX}/lib/libssl.3.dylib" "${DEST_FRAMEWORKS}/"
  cp "${OPENSSL_PREFIX}/lib/libcrypto.3.dylib" "${DEST_FRAMEWORKS}/"

  # Make them writable for install_name_tool
  chmod +w "${DEST_FRAMEWORKS}/libssl.3.dylib"
  chmod +w "${DEST_FRAMEWORKS}/libcrypto.3.dylib"

  # Fix IDs of the copied libs
  install_name_tool -id "@loader_path/libssl.3.dylib" "${DEST_FRAMEWORKS}/libssl.3.dylib"
  install_name_tool -id "@loader_path/libcrypto.3.dylib" "${DEST_FRAMEWORKS}/libcrypto.3.dylib"

  # Fix dependency of libssl on libcrypto - use @loader_path (they are in same dir)
  install_name_tool -change "${OPENSSL_PREFIX}/lib/libcrypto.3.dylib" "@loader_path/libcrypto.3.dylib" "${DEST_FRAMEWORKS}/libssl.3.dylib"

  # Now find the python extensions that need them (_ssl, _hashlib)
  # They are in lib-dynload
  local DYNLOAD_DIR
  DYNLOAD_DIR=GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/Current/lib/python${PYTHON_VERSION}/lib-dynload

  for EXT in _ssl _hashlib; do
    # Find the actual so file (e.g. _ssl.cpython-311-darwin.so)
    local EXT_FILE
    EXT_FILE=$(find "${DYNLOAD_DIR}" -name "${EXT}.*.so" -print -quit)

    log "Patching ${EXT_FILE}"
    # Update linkage to find libssl/libcrypto relative to _ssl.so
    # _ssl.so is in .../lib/python3.11/lib-dynload
    # libssl is in .../Frameworks
    # Path is @loader_path/../../../../../../libssl.3.dylib
    install_name_tool -change "${OPENSSL_PREFIX}/lib/libssl.3.dylib" "@loader_path/../../../../../../libssl.3.dylib" "${EXT_FILE}"
    install_name_tool -change "${OPENSSL_PREFIX}/lib/libcrypto.3.dylib" "@loader_path/../../../../../../libcrypto.3.dylib" "${EXT_FILE}"
  done

  # Fix missing QtDBus framework and libdbus.dylib (required by QtGui but missed by macdeployqt)
  # We copy it manually so it's present when we sign
  log "Manually copying QtDBus framework..."
  cp -R "$(realpath "$(brew --prefix qt)"/lib/QtDBus.framework)" GoldenCheetah.app/Contents/Frameworks/
  find "$(brew --prefix dbus)"/lib -maxdepth 1 -name 'libdbus-1*.dylib' -exec cp {} GoldenCheetah.app/Contents/Frameworks/ \;
  fix_binary_deps "GoldenCheetah.app/Contents/Frameworks/QtDBus.framework/Versions/A/QtDBus"

  # Deployment using macdeployqt - prepare bundle only
  macdeployqt GoldenCheetah.app -verbose=2 -executable=GoldenCheetah.app/Contents/MacOS/GoldenCheetah

  ### MANUAL LEAK PATCHING ###
  log "Starting manual leak patching..."

  # 0. cleanup static archives
  find GoldenCheetah.app/Contents/Frameworks -name "*.a" -delete

  # 1. QtWebEngineProcess
  local QWEBVIEW_APP
  QWEBVIEW_APP=GoldenCheetah.app/Contents/Frameworks/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess
  log "Patching QtWebEngineProcess..."
  install_name_tool -add_rpath "@executable_path/../../../../../../../" "${QWEBVIEW_APP}" || true
  fix_binary_deps "${QWEBVIEW_APP}"

  # 2. Python Binaries
  log "Patching Python binaries..."
  # Manual RPATH fix for python binaries so they can find @rpath/Python.framework...
  install_name_tool -add_rpath "@executable_path/../.." "${PYTHON_BIN}" || true
  fix_binary_deps "${PYTHON_BIN}"

  local PYTHON_APP_BIN
  PYTHON_APP_BIN=GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/Current/Resources/Python.app/Contents/MacOS/Python
  install_name_tool -add_rpath "@executable_path/../../../../../../.." "${PYTHON_APP_BIN}" || true
  fix_binary_deps "${PYTHON_APP_BIN}"

  # 3. Mass Scan
  log "Scanning entire bundle for other leaks..."
  find \
    GoldenCheetah.app/Contents/MacOS GoldenCheetah.app/Contents/Frameworks \
    \( -name "GoldenCheetah" -o -name "*.dylib" -o -name "*.so" -o -perm +111 \) \
    -type f \
    -exec bash -c 'file "$1" | grep -q Match-O' _ {} \; \
    -print0 |
    while read -r -d '' BINARY; do
      fix_binary_id "${BINARY}"
      fix_binary_deps "${BINARY}"
    done

  log "Resigning application bundle..."
  # Explicitly sign the Python components first to fix invalid Ad-Hoc signatures caused by install_name_tool
  log "Forcing signature refresh on Python components..."
  codesign --force --sign - "GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/Current/Python"
  codesign --force --sign - "GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python${PYTHON_VERSION}"
  codesign --force --sign - "${PYTHON_APP_BIN}"

  # Explicitly resign all .so and .dylib files in the framework (e.g. in lib-dynload)
  # codesign --deep on the app bundle often skips these or fails to resign them properly
  log "Resigning all dynamic libraries in Python framework AND Contents/Frameworks..."
  find "GoldenCheetah.app/Contents/Frameworks" -type f \( -name "*.dylib" -o -name "*.so" \) -exec codesign --force --sign - {} \;

  # Sign the nested Python.app if it exists (Inside-Out signing)
  local PYTHON_APP
  PYTHON_APP=GoldenCheetah.app/Contents/Frameworks/Python.framework/Versions/Current/Resources/Python.app
  log "Signing nested Python.app..."
  codesign --force --preserve-metadata=identifier,entitlements --sign - "${PYTHON_APP}"

  # Sign the Python framework itself
  # We verified structure earlier.
  log "Signing Python.framework..."
  codesign --force --sign - "GoldenCheetah.app/Contents/Frameworks/Python.framework"

  # - sign with ad-hoc identity check, this is free and works for local dev
  # - force rewrite of existing signatures (invalidated by install_name_tool)
  # - deep sign frameworks and plugins
  log "Signing final GoldenCheetah.app..."
  codesign --force --deep --sign - GoldenCheetah.app

  log "Preparing dmg contents"

  local DMG_NAME=GoldenCheetah_v3.8_arm64.dmg
  rm -rf dmg_staging
  mkdir -p dmg_staging
  ln -s /Applications dmg_staging/Applications
  cp -a GoldenCheetah.app dmg_staging/
  cp ../.github/scripts/DMG_README.md dmg_staging/README.md

  log "Creating dmg file..."
  hdiutil create -volname GoldenCheetah -srcfolder dmg_staging -format UDBZ "../${DMG_NAME}"
}
main "$@"

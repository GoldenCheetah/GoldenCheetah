#!/usr/bin/env bash

set -Eeuf -o pipefail
set -x

log() {
  printf '%s\n' "$*" >&2
}

err() {
  log "$*"
  exit 1
}

main() {
  case "${1:-}" in
    "")
      :
    ;;
    clean)
    make clean || true
    ;;
    *)
    err "unrecognized argument: $1"
  esac
 
  export HOMEBREW_NO_AUTO_UPDATE=1
  export HOMEBREW_NO_INSTALL_CLEANUP=1

  export OS_NAME=macos
  export PATH=/opt/homebrew/opt/bison/bin:/opt/homebrew/opt/python@${PYTHON_VERSION}/libexec/bin:$PATH
  export PYTHON_VERSION=3.11
  export QTDIR=/opt/homebrew

  local brewdeps=(
    automake
    bison
    gsl
    libical
    libsamplerate
    libtool
    libusb
    python@"${PYTHON_VERSION}"
  )

  brew install "${brewdeps[@]}"

  bash appveyor/macos/install.sh
  bash appveyor/macos/before_build.sh
 
  qmake build.pro -r \
    QMAKE_CXXFLAGS_WARN_ON+="-Wno-unused-private-field -Wno-c++11-narrowing -Wno-deprecated-declarations -Wno-deprecated-register -Wno-nullability-completeness -Wno-sign-compare -Wno-inconsistent-missing-override" \
    QMAKE_CFLAGS_WARN_ON+="-Wno-deprecated-declarations -Wno-sign-compare"
  if test ! -f qwt/lib/libqwt.a; then make -j2 sub-qwt; fi
  make -j2 sub-src

  bash appveyor/macos/after_build.sh
}

main "$@"

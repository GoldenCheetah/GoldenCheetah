#!/usr/bin/env bash
# shellcheck enable=all
# shellcheck shell=bash

set -Eeuf -o pipefail

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
      git clean -fdX
      local artifacts=(
        GoldenCheetah_v3.8_arm64.dmg
        src/GoldenCheetah.app
      )
      rm -rf -- "${artifacts[@]}"
      ;;
    clean-all)
      make clean || true
      git clean -fdX
      local cached_downloads=(
        D2XX1.4.24.dmg
        D2XX1.4.24.zip
        D2XX
        R-4.1.1-arm64.pkg
        srmio
      )
      rm -rf -- "${cached_downloads[@]}"
      ;;
    *)
      err "unrecognized argument: $1"
      ;;
  esac

  export \
    HOMEBREW_NO_AUTO_UPDATE=1 \
    HOMEBREW_NO_INSTALL_CLEANUP=1 \
    PATH=/opt/homebrew/opt/bison/bin:/opt/homebrew/opt/python@${PYTHON_VERSION}/libexec/bin:${PATH}

  local brewdeps=(
    automake
    bison
    gsl
    libical
    libsamplerate
    libtool
    libusb
    python@"${PYTHON_VERSION}"
    qt6
  )

  brew install "${brewdeps[@]}"

  .github/scripts/install.sh
  .github/scripts/before_build.sh

  qmake build.pro -r \
    QMAKE_CXXFLAGS_WARN_ON+="-Wno-unused-private-field -Wno-c++11-narrowing -Wno-deprecated-declarations -Wno-deprecated-register -Wno-nullability-completeness -Wno-sign-compare -Wno-inconsistent-missing-override" \
    QMAKE_CFLAGS_WARN_ON+="-Wno-deprecated-declarations -Wno-sign-compare"
  if [[ ! -f qwt/lib/libqwt.a ]]; then
    make -j2 sub-qwt
  fi
  make -j2 sub-src

  .github/scripts/after_build.sh
}

main "$@"

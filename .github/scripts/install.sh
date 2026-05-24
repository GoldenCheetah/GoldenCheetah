#!/usr/bin/env bash
# shellcheck enable=all
# shellcheck shell=bash

set -Eeuf -o pipefail

log() {
  printf '%s' "$*" >&2
}

err() {
  log "$*"
  exit 1
}

main() {
  # R 4.1.1
  if [[ ! -f R-4.1.1-arm64.pkg ]]; then
    curl -L -O https://cran.r-project.org/bin/macosx/big-sur-arm64/base/R-4.1.1-arm64.pkg
    sudo installer -pkg R-4.1.1-arm64.pkg -target /
    R --version
  fi

  # SRMIO
  if [[ ! -f srmio/.git/index ]]; then
    git clone https://github.com/rclasen/srmio.git
    pushd srmio
    sh genautomake.sh
    ./configure --disable-shared --enable-static --prefix=/opt/homebrew
    make -j2 --silent
    popd
  fi
  pushd srmio
  make install
  popd

  # D2XX - refresh cache if shared lib not found
  # libftd2xx.a specifically gets cleaned by `git clean -fdX`
  if [[ ! -f D2XX/libftd2xx.a ]]; then
    local d2xx_ver=1.4.24
    local d2xx_zip=D2XX"${d2xx_ver}".zip
    local d2xx_dmg=D2XX"${d2xx_ver}".dmg

    rm -rf D2XX "${d2xx_dmg}" "${d2xx_zip}"

    if [[ "${GITHUB_ACTIONS:-}" = true ]]; then
      unzip -d . vendor/"${d2xx_zip}"
    else
      curl -O https://ftdichip.com/wp-content/uploads/2021/05/"${d2xx_zip}"
      # cloudflare scraping protection is sometimes refusing to download from CI
      # in which case the zip file is an HTML error page
      if sha256 --check f59d18c11ecf5dedf0fcbdef24f18823c122ff24189a8e204479f9c408af7704 D2XX1.4.24.zip; then
        unzip "${d2xx_zip}"
      else
        err "checksum for ${d2xx_zip} incorrect"
      fi
    fi

    local mpoint
    mpoint=$(
      hdiutil mount "${d2xx_dmg}" -plist |
        xmllint -xpath '//key[text()="mount-point"]/following-sibling::string[1]/text()' -
    )
    # shellcheck disable=SC2064
    trap "hdiutil eject '${mpoint}'" EXIT

    mkdir -p D2XX
    cp \
      /Volumes/dmg/release/build/libftd2xx."${d2xx_ver}".dylib \
      /Volumes/dmg/release/build/libftd2xx.a \
      /Volumes/dmg/release/ftd2xx.h \
      /Volumes/dmg/release/WinTypes.h \
      D2XX
  fi
}
main "$@"

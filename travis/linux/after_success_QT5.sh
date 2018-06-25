#!/bin/bash

set -x
set -e

curl --upload-file "${TRAVIS_BUILD_DIR}/GoldenCheetah-${VERSION}-x86_64.AppImage" https://transfer.sh/GoldenCheetah-${VERSION}-x86_64.AppImage

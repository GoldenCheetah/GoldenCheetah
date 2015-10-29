#!/bin/bash
if [ "$BRANCH" = "master" ]
then
  $TRAVIS_BUILD_DIR/$BRANCH/src/GoldenCheetah.app/Contents/MacOS/GoldenCheetah --version
fi

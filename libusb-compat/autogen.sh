#!/bin/sh

set -e

./bootstrap.sh
./configure --enable-examples-build $*

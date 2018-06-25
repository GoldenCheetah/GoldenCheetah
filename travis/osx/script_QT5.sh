#!/bin/bash

set -x
set -e

CC=clang CXX=clang++ /usr/local/opt/$QT_PATH/bin/qmake -makefile -recursive QMAKE_CXXFLAGS_WARN_ON+="-Wno-unused-private-field -Wno-c++11-narrowing"
CC=clang CXX=clang++ make qmake_all
CC=clang CXX=clang++ make -j4 sub-qwt --silent
CC=clang CXX=clang++ make -j4 sub-src

#!/bin/bash
set -ev
# Build only if a package is not already cached
if [ -z "$(ls -A src/GoldenCheetah.app)" ]; then
CC=clang CXX=clang++ /usr/local/opt/qt5/bin/qmake -makefile -recursive QMAKE_CXXFLAGS_WARN_ON+="-Wno-unused-private-field -Wno-c++11-narrowing -Wno-deprecated-declarations -Wno-deprecated-register -Wno-nullability-completeness -Wno-sign-compare -Wno-inconsistent-missing-override" QMAKE_CFLAGS_WARN_ON+="-Wno-deprecated-declarations -Wno-sign-compare"
CC=clang CXX=clang++ make qmake_all
if [ -z "$(ls -A qwt/lib)" ]; then
CC=clang CXX=clang++ make -j4 sub-qwt
exit # build qwt and store in the cache on the first run
fi
CC=clang CXX=clang++ make -j4 sub-src
fi
exit

#!/bin/bash
set -ev
CC=clang CXX=clang++ /usr/local/opt/qt5/bin/qmake -makefile -recursive QMAKE_CXXFLAGS_WARN_ON+="-Wno-unused-private-field -Wno-c++11-narrowing -Wno-deprecated-declarations -Wno-deprecated-register -Wno-nullability-completeness -Wno-sign-compare -Wno-inconsistent-missing-override" QMAKE_CFLAGS_WARN_ON+="-Wno-deprecated-declarations -Wno-sign-compare"
CC=clang CXX=clang++ make qmake_all
CC=clang CXX=clang++ make -j4 sub-qwt --silent
CC=clang CXX=clang++ make -j4 sub-src --silent || CC=clang CXX=clang++ make sub-src
exit

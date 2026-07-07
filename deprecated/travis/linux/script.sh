#!/bin/bash
set -ev
qmake -recursive QMAKE_CXXFLAGS_WARN_ON+="-Wno-deprecated-declarations -Wno-unused-value" QMAKE_CFLAGS_WARN_ON+="-Wno-deprecated-declarations -Wno-unused-value"
make -j4
exit

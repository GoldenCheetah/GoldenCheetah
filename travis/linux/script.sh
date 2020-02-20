#!/bin/bash
set -ev
export PATH=/opt/qt514/bin:$PATH
qmake -recursive QMAKE_CXXFLAGS_WARN_ON+="-Wno-deprecated-declarations"
make --silent -j3 || make
exit

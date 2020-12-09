#!/bin/bash

SRC_DIR=$1
cat << EOF
[Desktop Entry]
  Name=GoldenCheetah
  Comment=Built for suffering
  Icon=/usr/share/pixmaps/openbox.xpm
  Exec=$SRC_DIR/GoldenCheetah/src/GoldenCheetah
  Type=Application
  Encoding=UTF-8
  Terminal=false
  StartUpNotify=true
  Categories=None;
EOF

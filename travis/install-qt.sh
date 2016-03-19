#!/bin/bash
if [ "$QT" = "qt5" ]
then
  pwd
  workdir=`pwd`
  echo ${workdir}
  cd $( brew --prefix )
  pwd
  # Select QT 5.5.1
  git checkout fb64f6cd91ff Library/Formula/qt5.rb
  cd ${workdir}
  pwd
fi
brew install $QT

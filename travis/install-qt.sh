#!/bin/bash
if [ "$QT" = "qt5" ]
then
  pwd
  workdir=`pwd`
  echo ${workdir}
  cd $( brew --prefix )
  pwd
  # Select QT 5.4.2
  git checkout 00e46351980 Library/Formula/qt5.rb
  cd ${workdir}
  pwd
fi
brew install $QT

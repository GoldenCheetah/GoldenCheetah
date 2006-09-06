#!/bin/sh
# $Id: addqt.sh,v 1.1 2006/08/11 20:01:16 srhea Exp $
QT_PATH=/usr/local/Trolltech/Qt-4.1.1
APP=GoldenCheetah
rm -rf $APP.app/Contents/Frameworks
mkdir -p $APP.app/Contents/Frameworks
cp -R $QT_PATH/lib/QtCore.framework $APP.app/Contents/Frameworks
cp -R $QT_PATH/lib/QtGui.framework $APP.app/Contents/Frameworks
install_name_tool -id @executable_path/../Frameworks/QtCore.framework/Versions/4.0/QtCore $APP.app/Contents/Frameworks/QtCore.framework/Versions/4.0/QtCore
install_name_tool -id @executable_path/../Frameworks/QtGui.framework/Versions/4.0/QtGui $APP.app/Contents/Frameworks/QtGui.framework/Versions/4.0/QtGui 
install_name_tool -change $QT_PATH/lib/QtCore.framework/Versions/4.0/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4.0/QtCore $APP.app/Contents/MacOs/$APP
install_name_tool -change $QT_PATH/lib/QtGui.framework/Versions/4.0/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4.0/QtGui $APP.app/Contents/MacOs/$APP
install_name_tool -change $QT_PATH/lib/QtCore.framework/Versions/4.0/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4.0/QtCore $APP.app/Contents/Frameworks/QtGui.framework/Versions/4.0/QtGui

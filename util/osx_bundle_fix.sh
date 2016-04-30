#!/bin/sh
LOCAL_RLIBS=`otool -L GoldenCheetah.app/Contents/MacOS/GoldenCheetah | grep R.framework | awk '{ print $1}'`

for lib in $LOCAL_RLIBS
do
    echo cp $lib GoldenCheetah.app/Contents/MacOS
    cp $lib GoldenCheetah.app/Contents/MacOS
    echo install_name_tool -change $lib "@executable_path/"`basename $lib` GoldenCheetah.app/Contents/MacOS/GoldenCheetah
    install_name_tool -change $lib "@executable_path/"`basename $lib` GoldenCheetah.app/Contents/MacOS/GoldenCheetah
done

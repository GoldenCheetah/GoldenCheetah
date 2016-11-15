#!/bin/sh

localise() {

    if [ $2 -gt 4 ]; then
         return
    fi

    # increase a level
    target=$1
    level=$2
    level=`expr $level + 1`

    echo "level=" $level "target=" $target

    for lib in `otool -L $target | grep R.framework | awk '{ print $1}'`
    do

        # copy if its not there
        if [ ! -e GoldenCheetah.app/Contents/MacOS/`basename $lib` ]; then
            cp $lib GoldenCheetah.app/Contents/MacOS
        fi

        echo install_name_tool -change $lib "@executable_path/"`basename $lib` $target
        install_name_tool -change $lib "@executable_path/"`basename $lib` $target

        # go down a level
        localise GoldenCheetah.app/Contents/MacOS/`basename $lib` $level
    done
}

localise GoldenCheetah.app/Contents/MacOS/GoldenCheetah 1

# now all the things we copied
for i in GoldenCheetah.app/Contents/MacOS/*.dylib
do
    localise $i 1
done

echo "****       you may need to run this a few times         ****"
echo "run otool -L on the contents of the MacOS directory to check"
echo

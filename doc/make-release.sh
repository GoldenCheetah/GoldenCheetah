#!/bin/sh
# $Id: mkdmg.sh,v 1.2 2006/09/06 23:23:03 srhea Exp $
export PATH=/usr/local/Trolltech/Qt-4.1.1-static/bin:$PATH
VERS=`date +'%Y-%m-%d'`
OS=`uname -s`
CPU=`uname -p`
SUFFIX="$VERS"_"$OS"_"$CPU"
rm -rf tmp
mkdir tmp
cd tmp
svn checkout svn+ssh://goldencheetah.org/home/srhea/svnroot/goldencheetah/trunk goldencheetah
cd goldencheetah
make
mv src/gui/GoldenCheetah.app ..
make clean
rm doc/gc_*.tgz
rm doc/GoldenCheetah_*.dmg
rm doc/GoldenCheetah_*.tgz
cd ..
strip GoldenCheetah.app/Contents/MacOS/GoldenCheetah
find . -name .svn | xargs rm -rf
tar czvf src.tgz goldencheetah
rm -rf goldencheetah
SIZE=`du -csk * | grep total | awk '{printf "%.0fm", $1/1024+5}'`
hdiutil create -size $SIZE -fs HFS+ -volname "Golden Cheetah $VERS" tmp.dmg
hdiutil attach tmp.dmg
cp -R GoldenCheetah.app src.tgz /Volumes/Golden\ Cheetah\ $VERS/
hdiutil detach /Volumes/Golden\ Cheetah\ $VERS/
hdiutil convert tmp.dmg -format UDZO -o GoldenCheetah_$SUFFIX.dmg
hdiutil internet-enable -yes GoldenCheetah_$SUFFIX.dmg
cd ..
mv tmp/GoldenCheetah_$SUFFIX.dmg .
rm -rf tmp

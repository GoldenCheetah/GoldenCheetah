#!/bin/sh
# $Id: mkdmg.sh,v 1.2 2006/09/06 23:23:03 srhea Exp $
CMD_FILES="COPYING ptdl ptunpk intervals.pl smooth.pl cpint ptpk"
GUI_FILE="GoldenCheetah.app"
VERS=`date +'%Y-%m-%d'`
strip GoldenCheetah.app/Contents/MacOS/GoldenCheetah
rm -rf tmp.dmg
SIZE=`cd ..; du -csk $CMD_FILES gui/GoldenCheetah.app | grep total | awk '{printf "%.0fm", $1/1024+5}'`
echo "SIZE=$SIZE, VERS=$VERS"
hdiutil create -size $SIZE -fs HFS+ -volname "Golden Cheetah $VERS" tmp.dmg
hdiutil attach tmp.dmg
cp -R GoldenCheetah.app /Volumes/Golden\ Cheetah\ $VERS/
cd .. && cp $CMD_FILES /Volumes/Golden\ Cheetah\ $VERS/ && cd -
hdiutil detach /Volumes/Golden\ Cheetah\ $VERS/
hdiutil convert tmp.dmg -format UDZO -o GoldenCheetah_$VERS.dmg
hdiutil internet-enable -yes GoldenCheetah_$VERS.dmg
rm -rf tmp.dmg

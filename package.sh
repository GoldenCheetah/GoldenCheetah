#!/bin/sh
VERSION=`date +'%Y-%m-%d'`
FILES="COPYING Makefile pt.h pt.c ptdl.c ptunpk.c intervals.pl smooth.pl"
FILES="$FILES cpint.c cpint.h cpint-cmd.c ptpk.c"
mkdir gc_$VERSION
cp $FILES gc_$VERSION
tar cvzf gc_$VERSION.tgz gc_$VERSION 
rm -rf gc_$VERSION

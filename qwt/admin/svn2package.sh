#! /bin/sh
# 
# Generates a Qwt package from sourceforge svn
#
# Usage: svn2package.sh [-b|--branch <svn-branch>] [packagename] 
# 

##########################
# usage
##########################

function usage() {
    echo "Usage: $0 [-b|--branch <svn-branch>] [-s|--suffix <suffix>] [-html] [-pdf] [-qch] [packagename]"
    exit 1
}

################################
# checkout 
################################

function checkoutQwt() {

    if [ -x $2 ]
    then
        rm -r $2
        if [ $? -ne 0 ]
        then
            exit $?
        fi
    fi

    svn -q co https://svn.code.sf.net/p/qwt/code/$1/$2
    if [ $? -ne 0 ]
    then
        echo "Can't access sourceforge SVN"
        exit $?
    fi

    if [ "$3" != "$2" ]
    then
        rm -rf $3
        mv $2 $3
    fi
}

##########################
# cleanQwt dirname
##########################

function cleanQwt {

    cd $1
    if [ $? -ne 0 ]
    then
        exit $?
    fi

    find . -name .svn -print | xargs rm -r

    rm -f TODO
    rm -rf admin
    rm -rf doc/tex

    PROFILES="qwtbuild.pri"
    for PROFILE in $PROFILES
    do
        sed -i -e 's/= debug/= release/' $PROFILE 
        sed -i -e 's/= release_and_release/= debug_and_release/' $PROFILE 
    done

    HEADERS=`find . -type f -name '*.h' -print`
    SOURCES=`find . -type f -name '*.cpp' -print`
    PROFILES=`find . -type f -name '*.pro' -print`
    PRIFILES=`find . -type f -name '*.pri' -print`

    for EXPANDFILE in $HEADERS $SOURCES $PROFILES $PRIFILES
    do
        expand -4 $EXPANDFILE > $EXPANDFILE.expand
        mv $EXPANDFILE.expand $EXPANDFILE
    done

    for SRCFILE in $SOURCES $PROFILES $PRIFILES
    do
        sed -i -e '/#warning/d' $SRCFILE 
    done 

    if [ "$SUFFIX" != "" ]
    then
        sed -i -e "s/\$\$QWT_VERSION-svn/\$\$QWT_VERSION-$SUFFIX/" qwtconfig.pri 
    	sed -i -e "s/\$(QWTVERSION)/$VERSION-$SUFFIX/" doc/install.dox
    else
        sed -i -e "s/\$\$QWT_VERSION-svn/\$\$QWT_VERSION/" qwtconfig.pri
    	sed -i -e "s/\$(QWTVERSION)/$VERSION/" doc/install.dox
    fi

    cd - > /dev/null
}

##########################
# createDocs dirname
##########################

function createDocs {

    ODIR=`pwd`

    cd $1
    if [ $? -ne 0 ]
    then
        exit $?
    fi

    if [ "$SUFFIX" != "" ]
    then
		export QWTVERSION=$VERSION-$SUFFIX
    else
		export QWTVERSION=$VERSION
    fi
    cp Doxyfile Doxyfile.doc

    if [ $GENERATE_MAN -ne 0 ]
    then
        sed -i -e '/GENERATE_MAN/d' Doxyfile.doc
        echo 'GENERATE_MAN = YES' >> Doxyfile.doc
    fi

    if [ $GENERATE_PDF -ne 0 ]
    then
        # We need LateX for the qwtdoc.pdf

        sed -i -e '/GENERATE_LATEX/d' -e '/GENERATE_MAN/d' Doxyfile.doc
        echo 'GENERATE_LATEX = YES' >> Doxyfile.doc
        echo 'GENERATE_MAN = YES' >> Doxyfile.doc

#        sed -i -e '/INLINE_INHERITED_MEMB/d' Doxyfile.doc
#        echo 'INLINE_INHERITED_MEMB = NO' >> Doxyfile.doc
    fi

    if [ $GENERATE_QCH -ne 0 ]
    then
        sed -i -e '/GENERATE_QHP/d' Doxyfile.doc
        echo "GENERATE_QHP = YES" >> Doxyfile.doc
    fi

    cp ../INSTALL ../COPYING ./

    doxygen Doxyfile.doc > /dev/null 2>&1
    if [ $? -ne 0 ]
    then
        exit $?
    fi

    rm Doxyfile.doc Doxygen.log INSTALL COPYING 
    rm -r images

    if [ $GENERATE_PDF -ne 0 ]
    then
        cd latex
        make > /dev/null 2>&1
        if [ $? -ne 0 ]
        then 
            exit $?
        fi

        cd ..
        mkdir pdf
        mv latex/refman.pdf pdf/qwtdoc-$VERSION.pdf

        rm -r latex 
    fi
    
    cd $ODIR
}

##########################
# posix2dos filename
##########################

function posix2dos {
    # At least one unix2dos writes to stdout instead of overwriting the input.
    # The -q option is always enabled in stdin->stdout mode.
    unix2dos <$1 >$1.dos
    mv $1.dos $1
}

##########################
# prepare4Win dirname
##########################

function prepare4Win {

    cd $1
    if [ $? -ne 0 ]
    then
        exit $?
    fi

    rm -rf doc/man 2> /dev/null

    # win files, but not uptodate

    BATCHES=`find . -type f -name '*.bat' -print`
    HEADERS=`find . -type f -name '*.h' -print`
    SOURCES=`find . -type f -name '*.cpp' -print`
    PROFILES=`find . -type f -name '*.pro' -print`
    PRIFILES=`find . -type f -name '*.pri' -print`
    PRFFILES=`find . -type f -name '*.prf' -print`

    for FILE in $BATCHES $HEADERS $SOURCES $PROFILES $PRIFILES $PRFFILES
    do
        posix2dos $FILE
    done

    cd - > /dev/null
}

##########################
# prepare4Unix dirname
##########################

function prepare4Unix {

    cd $1
    if [ $? -ne 0 ]
    then
        exit $?
    fi

    cd - > /dev/null
}

##########################
# main
##########################

QWTDIR=
SVNDIR=trunk
BRANCH=qwt
SUFFIX=
VERSION=
GENERATE_DOC=0
GENERATE_PDF=0
GENERATE_QCH=0
GENERATE_MAN=0

while [ $# -gt 0 ] ; do
    case "$1" in
        -h|--help)
            usage; exit 1 ;;
        -b|--branch)
            shift; SVNDIR=branches; BRANCH=$1; shift;;
        -s|--suffix)
            shift; SUFFIX=$1; shift;;
        -html)
            GENERATE_DOC=1; shift;;
        -pdf)
            GENERATE_DOC=1; GENERATE_PDF=1; shift;;
        -qch)
            GENERATE_DOC=1; GENERATE_QCH=1; shift;;
        *) 
            QWTDIR=qwt-$1 ; VERSION=$1; shift;;
    esac
done

if [ "$QWTDIR" == "" ] 
then 
    usage 
    exit 2 
fi

QWTNAME=$QWTDIR
if [ "$SUFFIX" != "" ]
then
    QWTDIR=$QWTDIR-$SUFFIX
fi

TMPDIR=/tmp/$QWTDIR-tmp

echo -n "checkout to $TMPDIR ... "
checkoutQwt $SVNDIR $BRANCH $TMPDIR
cleanQwt $TMPDIR
echo done

if [ $GENERATE_DOC -ne 0 ]
then
    echo -n "generate documentation ... "

	export VERSION # used in the doxygen files
    createDocs $TMPDIR/doc

    if [ $GENERATE_PDF -ne 0 ]
    then
        mv $TMPDIR/doc/pdf/qwtdoc-$VERSION.pdf $QWTDIR.pdf
        rmdir $TMPDIR/doc/pdf
    fi

    if [ $GENERATE_QCH -ne 0 ]
    then
        mv $TMPDIR/doc/html/qwtdoc.qch $QWTDIR.qch
    fi
fi

echo done


DIR=`pwd`
echo -n "create packages in $DIR ... "

cd /tmp

rm -rf $QWTDIR
cp -a $TMPDIR $QWTDIR
prepare4Unix $QWTDIR
tar cfj $QWTDIR.tar.bz2 $QWTDIR

rm -rf $QWTDIR
cp -a $TMPDIR $QWTDIR
prepare4Win $QWTDIR
zip -r $QWTDIR.zip $QWTDIR > /dev/null

rm -rf $TMPDIR $QWTDIR

mv $QWTDIR.tar.bz2 $QWTDIR.zip $DIR/
echo done

exit 0

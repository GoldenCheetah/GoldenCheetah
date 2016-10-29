# NOTE: This is a major hack: qmake is not supposed to be used in that way (integrating an
# autotools project).
# Also: The targets 'install', 'uninstall', 'dist' and 'distclean' are not accounted for and their
# behavoir is undefined. The only thing this project does, is building the static lib 'libusb.a',
# which is then used by GC.

include(../src/gcconfig.pri)

# Hack: Using subdirs template enables build in QtCreator
TEMPLATE = subdirs

MAKEFILE = Makefile.phony

!isEmpty(LIBUSB_INSTALL) {

    # libusb-1.0
    # we will work out the rest if you tell use where it is installed
    isEmpty(LIBUSB_INCLUDE) { LIBUSB_INCLUDE = $${LIBUSB_INSTALL}/include/libusb-1.0 }
    isEmpty(LIBUSB_LIBS)    { LIBUSB_LIBS = -L$${LIBUSB_INSTALL}/lib -lusb-1.0 }

    # pass LIBUSB paths from gcconfig.pri to autoconf
    system(LIBUSB_1_0_CFLAGS=-I$${LIBUSB_INCLUDE} LIBUSB_1_0_LIBS=\"$${LIBUSB_LIBS}\" ./autogen.sh --enable-shared=no)
}

isEmpty(LIBUSB_INSTALL) {
    # create dummy Makefile (with empty targets), so that overall build is not disturbed
    system(cp Makefile.dummy Makefile)
}


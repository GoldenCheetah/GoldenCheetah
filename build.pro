TEMPLATE = subdirs
SUBDIRS = qwt
unix:!macx {
    SUBDIRS += kqoauth libusb-compat
}
SUBDIRS += src
CONFIG += ordered

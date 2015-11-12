TEMPLATE = subdirs
SUBDIRS = qwt src
unix {
    SUBDIRS += kqoauth
}
CONFIG += ordered

TEMPLATE = subdirs
SUBDIRS = qwt
unix:!macx {
    SUBDIRS += kqoauth
}
SUBDIRS += src
CONFIG += ordered

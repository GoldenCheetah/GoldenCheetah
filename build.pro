TEMPLATE = subdirs
SUBDIRS = qwt src
unix:!macx {
    SUBDIRS += kqoauth
}
CONFIG += ordered

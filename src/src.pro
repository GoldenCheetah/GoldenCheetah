TEMPLATE = subdirs
CONFIG += ordered
macx || unix {
    SUBDIRS = lib cmd
}
SUBDIRS = srm pt gui

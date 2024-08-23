TEMPLATE = subdirs

exists(unittests.pri) {
	include(unittests.pri)
}

equals(GC_UNITTESTS, active) {
	SUBDIRS += seasonOffset \
			   season \
			   seasonParser
	CONFIG += ordered
} else {
	message("Unittests are disabled; to enable copy unittests/unittests.pri.in to unittests/unittests.pri")
}

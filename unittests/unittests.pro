TEMPLATE = subdirs

exists(unittests.pri) {
	include(unittests.pri)
}

equals(GC_UNITTESTS, active) {
	SUBDIRS += Core/seasonOffset \
			   Core/season \
			   Core/seasonParser \
			   Core/units \
			   Gui/calendarData
	CONFIG += ordered
} else {
	message("Unittests are disabled; to enable copy unittests/unittests.pri.in to unittests/unittests.pri")
}

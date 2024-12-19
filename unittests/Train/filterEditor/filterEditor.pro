QT += testlib widgets

SOURCES = testFilterEditor.cpp
GC_OBJS = FilterEditor \
	      moc_FilterEditor

include(../../unittests.pri)

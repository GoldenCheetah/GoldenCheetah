TEMPLATE = subdirs

exists(unittests.pri) {
	include(unittests.pri)
}

equals(GC_UNITTESTS, active) {
	SUBDIRS += Core/seasonOffset \
			   Core/season \
			   Core/seasonParser \
			   Core/units \
			   Core/utils \
			   Core/signalSafety \
			   Core/splineCrash \
			   Core/trainingConstraints \
			   Core/trainingSimulator \
			   Core/trainingPlan \
			   Cloud/jsonParsers \
			   Cloud/stravaCredentials \
			   FileIO/computrainer3dp \
			   FileIO/tacxCaf \
			   FileIO/ttsReader \
			   FileIO/rideAutoImport \
			   Gui/calendarData
	CONFIG += ordered
} else {
	message("Unittests are disabled; to enable copy unittests/unittests.pri.in to unittests/unittests.pri")
}

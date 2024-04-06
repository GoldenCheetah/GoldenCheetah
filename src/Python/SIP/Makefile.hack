#
# Run 'make' manually to regenerate the SRC files
# at some point later we will integrate into src.pro
#

SRC= sipgoldencheetahBindings.cpp sipgoldencheetahcmodule.cpp sipAPIgoldencheetah.h \
     sipgoldencheetahQString.cpp sipgoldencheetahQStringRef.cpp sipgoldencheetahQStringList.cpp

DEPS= goldencheetah.sip

$(SRC): $(DEPS)
	sip -c . goldencheetah.sip

clean: 
	-rm $(SRC)


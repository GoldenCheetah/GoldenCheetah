SIP_BUILD_CMD = sip-build
TARGET = sipgoldencheetahBindings.cpp
DEPS = goldencheetah.sip

all: $(TARGET)

$(TARGET): $(DEPS)
	@echo "Generating SIP bindings..."
	$(SIP_BUILD_CMD) --no-compile
	@cp build/goldencheetah/* .
	@for f in sip*.cpp; do \
		sed -i.bak 's/#include "sipAPIgoldencheetah.h"/#undef slots\n#include "sipAPIgoldencheetah.h"\n#define slots Q_SLOTS/' $$f; \
		rm -f $$f.bak; \
	done
	@touch $(TARGET)

clean:
	rm -rf build sip*.cpp sip*.c sip*.h

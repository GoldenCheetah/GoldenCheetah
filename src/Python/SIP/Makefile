SIP_BUILD_CMD ?= sip-build
TARGET = sipgoldencheetahBindings.cpp
DEPS = goldencheetah.sip

all: $(TARGET)

$(TARGET): $(DEPS)
	@echo "Generating SIP bindings..."
	$(SIP_BUILD_CMD) --no-compile
	@cp build/goldencheetah/* .
	@touch $(TARGET)

clean:
	rm -rf build sip*.cpp sip*.c sip*.h

PLUGIN_NAME = mathulate
SOURCES = mathulate.cpp mathulate_core.cpp
DISTINGNT_API ?= ./distingNT_API
VERSION ?= $(shell git describe --tags --always --dirty 2>/dev/null || echo "v0.1.0-dev")
VERSION_DEFINE = -DMATHULATE_VERSION=\"$(VERSION)\"

UNAME_S := $(shell uname -s)
TARGET ?= hardware
HOST_CXX ?= $(if $(filter Darwin,$(UNAME_S)),clang++,g++)
HOST_CXXFLAGS ?= -std=c++11 -O0 -Wall -Wextra -I.
UNIT_OUTPUT = build/mathulate_core_test

ifeq ($(TARGET),hardware)
    CXX = arm-none-eabi-g++
    CXXFLAGS = -std=c++11 \
               -mcpu=cortex-m7 \
               -mfpu=fpv5-d16 \
               -mfloat-abi=hard \
               -mthumb \
               -Os \
               -fPIC \
               -ffunction-sections \
               -fdata-sections \
               -fno-rtti \
               -fno-exceptions \
               -fno-unwind-tables \
               -fno-asynchronous-unwind-tables \
               -Wall \
               $(VERSION_DEFINE)
    LDFLAGS = -Wl,--relocatable -nostdlib
    OUTPUT_DIR = plugins
    BUILD_DIR = build
    OUTPUT = $(OUTPUT_DIR)/$(PLUGIN_NAME).o
    OBJECTS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
    NM = arm-none-eabi-nm
    SIZE_CMD = arm-none-eabi-size -A $(OUTPUT)
else ifeq ($(TARGET),test)
    ifeq ($(UNAME_S),Darwin)
        CXX = clang++
        CXXFLAGS = -std=c++11 -fPIC -Os -Wall -fno-rtti -fno-exceptions $(VERSION_DEFINE)
        LDFLAGS = -dynamiclib -undefined dynamic_lookup
        EXT = dylib
    endif

    ifeq ($(UNAME_S),Linux)
        CXX = g++
        CXXFLAGS = -std=c++11 -fPIC -Os -Wall -fno-rtti -fno-exceptions $(VERSION_DEFINE)
        LDFLAGS = -shared
        EXT = so
    endif

    ifeq ($(OS),Windows_NT)
        CXX = g++
        CXXFLAGS = -std=c++11 -fPIC -Os -Wall -fno-rtti -fno-exceptions $(VERSION_DEFINE)
        LDFLAGS = -shared
        EXT = dll
    endif

    OUTPUT_DIR = plugins
    OUTPUT = $(OUTPUT_DIR)/$(PLUGIN_NAME).$(EXT)
    NM = nm
endif

INCLUDES = -I. -I$(DISTINGNT_API)/include

all: $(OUTPUT)

check-api:
	@test -f "$(DISTINGNT_API)/include/distingnt/api.h" || \
		(echo "Missing distingNT API headers. Set DISTINGNT_API=/path/to/distingNT_API or create ./distingNT_API."; exit 1)

ifeq ($(TARGET),hardware)
$(OUTPUT): check-api $(OBJECTS)
	@mkdir -p $(OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJECTS)
	@echo "Built: $@"
	@$(SIZE_CMD)

$(BUILD_DIR)/%.o: %.cpp Makefile | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
else ifeq ($(TARGET),test)
$(OUTPUT): check-api $(SOURCES)
	@mkdir -p $(OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $(SOURCES)
	@echo "Built: $@"
endif

hardware:
	@$(MAKE) TARGET=hardware

test:
	@$(MAKE) TARGET=test

both:
	@$(MAKE) TARGET=hardware
	@$(MAKE) TARGET=test

push: hardware
	ntpush $(OUTPUT_DIR)/$(PLUGIN_NAME).o

unit: $(UNIT_OUTPUT)
	@$(UNIT_OUTPUT)

$(UNIT_OUTPUT): tests/mathulate_core_test.cpp mathulate_core.cpp mathulate_core.h Makefile | $(BUILD_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) -o $@ tests/mathulate_core_test.cpp mathulate_core.cpp

verify:
	@$(MAKE) unit
	@$(MAKE) TARGET=hardware
	@$(MAKE) TARGET=test
	@$(MAKE) check TARGET=hardware

check: $(OUTPUT)
	@echo "Undefined symbols in $(OUTPUT):"
	@$(NM) -u $(OUTPUT) || true

size: $(OUTPUT)
	@$(SIZE_CMD)

clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_DIR)

.PHONY: all hardware test both push unit verify check size clean check-api

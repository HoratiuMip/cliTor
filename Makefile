BUILD_DIR   =  build
PREFIX      ?= /usr
CMAKE_FLAGS ?=

.PHONY: all config install wipe

all:
	@echo -n "[cliTor] compiling: "
	cd $(BUILD_DIR) && $(MAKE) -j

config:
	@echo -n "[cliTor] jumping into the build directory: "
	mkdir -p $(BUILD_DIR)

	@echo -n "[cliTor] running CMake script: "
	cd ${BUILD_DIR} && cmake -DCMAKE_INSTALL_PREFIX=$(PREFIX) ${CMAKE_FLAGS} ../src

install:
	@echo -n "[cliTor] installing: "
	cd $(BUILD_DIR) && sudo cmake --install . --component cliTor

wipe:
	@echo -n "[cliTor] cleaning: "
	rm -rf $(BUILD_DIR)

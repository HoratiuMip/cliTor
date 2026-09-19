# FILE: Makefile
#
# DESCRIPTION: Top level makefile for cliTor.
#
# MANUAL: When building cliTor using this makefile, you must call "make config", "make", and, optionally, "make install". 
#
#         Make sure you have the variable RGH_ROOT_DIR pointing to the root directory of regolith lain. You may also
#           pass -DRGH_ROOT_DIR=<path_to_dir> via CMAKE_ARGS.
#         You may pass extra desired arguments to CMake via CMAKE_ARGS.
#
#		  After the build is completed, you may call "make run" to execute your freshly built app.
#
# EXAMPLE:
#	[1]: make config CMAKE_ARGS="-DRGH_ROOT_DIR=/home/chill-dude/regolith-lain"
#        make
#        sudo make install
#
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu

BUILD_DIR  ?= build
PREFIX     ?= /usr
CMAKE_ARGS ?=

.PHONY: all config install clean run

all:
	@echo -n "[cliTor] compiling: "
	cd $(BUILD_DIR) && $(MAKE) -j

config:
	@echo -n "[cliTor] making the build directory: "
	mkdir -p $(BUILD_DIR)

	@echo -n "[cliTor] running CMake script: "
	cd ${BUILD_DIR} && cmake -DCMAKE_INSTALL_PREFIX=$(PREFIX) ${CMAKE_ARGS} ../src

install:
	@echo -n "[cliTor] installing: "
	cd $(BUILD_DIR) && cmake --install . --component cliTor

clean:
	@echo -n "[cliTor] cleaning: "
	rm -rf $(BUILD_DIR)

run:
	@echo -n "[cliTor] running: "
	./${BUILD_DIR}/cliTor

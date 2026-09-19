# FILE: Makefile
#
# DESCRIPTION: Top level makefile for cliTor.
#
# MANUAL: When building cliTor using this makefile, you must call "make config", "make", and, optionally, "make install". 
#         You may pass extra desired arguments to CMake via CMAKE_ARGS.
#		  After the build is completed, you may call "make run" to execute your freshly built app.
#
# EXAMPLE: [1]: make config && make && sudo make install
#          [2]: make config && make && make run
#
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu

BUILD_DIR  ?= build
PREFIX     ?= /usr
CMAKE_ARGS ?=
PY         ?= python

ifeq ( $(OS), Windows_NT )
	SHELL       := powershell.exe
    .SHELLFLAGS := -NoProfile -ExecutionPolicy Bypass -Command

	PRINT = Write-Host -NoNewLine
    MKDIR = mkdir -Force
    RMDIR = rm -r -fo
else
    PRINT = echo -n
    MKDIR = mkdir -p
    RMDIR = rm -rf
endif

.PHONY: all
all:
	@${PRINT} "[cliTor] compiling: "
	cd $(BUILD_DIR) && $(MAKE) -j

.PHONY: config
config:
	@${PRINT} "[cliTor] making the build directory: "
	${MKDIR} $(BUILD_DIR)

	@${PRINT} "[cliTor] running CMake script: "
	cd ${BUILD_DIR} && cmake -DCMAKE_INSTALL_PREFIX=$(PREFIX) ${CMAKE_ARGS} ../src

.PHONY: install
install:
	@${PRINT} "[cliTor] installing: "
	cd $(BUILD_DIR) && cmake --install . --component cliTor

.PHONY: clean
clean:
	@${PRINT} "[cliTor] cleaning: "
	${RMDIR} $(BUILD_DIR)

.PHONY: run
run:
	@${PRINT} "[cliTor] running: "
	./${BUILD_DIR}/cliTor

.PHONY: gui
gui:
	@${PY} make.py

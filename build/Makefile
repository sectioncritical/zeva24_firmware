# SPDX-License-Identifier: MIT
#
# Copyright 2021 Joseph Kroesche
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# using bashisms
SHELL=/bin/bash

PROGNAME?=zeva_bms_24

TARGET_MCU?=atmega16m1

# development version, override for production
VERSION?=99.99.99

# boot loader released version
BL_VERSION?=1.0.0

# extra defines to pass to compiler
DEFINES=-DF_CPU=8000000UL

# AVRDUDE
# used for direct programming the MCU flash memory
# This Makefile assumes the target is connected using "usbtiny" style
# programmer dongle.
#
# location of avrdude binary. If it is on your system path
# then you can just use AVRDUDE=avrdude
AVRDUDE?=~/.platformio/packages/tool-avrdude/bin/avrdude
# if your avrdude correctly resolve the config file location
# then this can be blank (AVRDUDE_CONF=)
AVRDUDE_CONF?=-C ~/.platformio/packages/tool-avrdude/avrdude.conf
# device and programmer settings
AVRDUDE_DEVICE?=atmega16m1
AVRDUDE_PROGRAMMER=usbtiny
AVRDUDE_BAUD=19200

AVRDUDE_CMD=$(AVRDUDE) -v -p $(AVRDUDE_DEVICE) $(AVRDUDE_CONF) -c $(AVRDUDE_PROGRAMMER)

OUT=obj
SRC=../src

OBJS=$(OUT)/bms24.o

# device remains unlocked
LOCKFUSE=0xff

### HFUSE configuration for system with boot loader
# the boot loader is separate and should be installed first
#
# leave reset enabled   : 1xxx xxxx
# debugwire disabled    : x1xx xxxx
# SPI prog enabled      : xx0x xxxx
# WDT not enabled       : xxx1 xxxx
# dont erase eeprom     : xxxx 0xxx
# boot size 2048        : xxxx x01x
# bootloader reset      : xxxx xxx0
# result                : 1101 0010 = 0xD2
HFUSE_BOOT=0xd2

### HFUSE configuration if no boot loader
# enable SPI memory access, EESAVE (dont erase eeprom), minimal boot size
HFUSE_NOBOOT=0xd7

# disable /8, external osc, longer startup time
LFUSE=0xdf

# BOD level 4.2
EFUSE=0xfa

HEXFILE:=$(OUT)/$(PROGNAME).hex
ELFFILE:=$(OUT)/$(PROGNAME).elf
UPLOADFILE?=$(HEXFILE)

all: $(HEXFILE)

.PHONY: help
help:
	@echo ""
	@echo "Zeva BMS24 Makefile Help"
	@echo "========================"
	@echo ""
	@echo "Development Host (Building and Programming)"
	@echo "-------------------------------------------"
	@echo "all/(default)    - build the BMS firmware (hex/elf)"
	@echo "clean            - clean the build products"
	@echo "realclean        - clean all build artifacts and packages"
	@echo ""
	@echo "program          - program hex file to target using programmer"
	@echo "program0         - program original legacy ZEVA code"
	@echo "erase            - erase target flash (not normally needed)"
	@echo "fuses-noboot     - program fuses for non-bootloader config"
	@echo "target-probe     - programmer probe the target"
	@echo ""
	@echo "Release Package"
	@echo "---------------"
	@echo "package          - create versioned archive (use VERSION=x.y.z)"
	@echo "program-pkg      - program from a release package (VERSION=x.y.z)"
	@echo ""
	@echo "Boot Loader"
	@echo "-----------"
	@echo "fuses-boot       - program fuses for boot loader configuration"
	@echo "program-boot     - download BL release and program it (BL_VERSION=1.0.0)"
	@echo ""
	@echo "Python Virtual Environment"
	@echo "--------------------------"
	@echo "venv             - create the python virtual environment"
	@echo "cleanvenv        - clean the python virtual environment"
	@echo ""

CC=avr-gcc
OBJCOPY=avr-objcopy
SIZE=avr-size

CFLAGS=-std=c99 -Os
CFLAGS+=-Wall -Werror
CFLAGS+=-mmcu=$(TARGET_MCU)
# turning off the following results in slightly smaller code
CFLAGS+=-ffunction-sections -fdata-sections -fshort-enums -flto
CFLAGS+=$(DEFINES)
LDFLAGS=-Wl,-Map,$(OUT)/$(PROGNAME).map -Wl,--gc-sections -fuse-linker-plugin

$(OUT):
	mkdir -p $@

$(OUT)/%.o: $(SRC)/%.c | $(OUT)
	$(CC) $(CFLAGS) -o $@  -c $<

$(ELFFILE): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBFLAGS)

$(HEXFILE): $(ELFFILE)
	$(OBJCOPY) -O ihex -R .eeprom $< $@
	$(SIZE) $<

.PHONY: clean
clean:
	rm -rf $(OUT)

.PHONY: realclean
realclean: clean
	rm -rf canboot-*
	rm -rf zeva_bms_24-*

# flash the firmware onto the target
.PHONY: program
program: $(HEXFILE)
	$(AVRDUDE_CMD) -U flash:w:$<:i

.PHONY: program0
program0:
	$(AVRDUDE_CMD) -U flash:w:zeva_v0/BMS24.hex:i

# dump the flash contents into a file
.PHONY: dumpflash
dumpflash:
	$(AVRDUDE_CMD) -U flash:r:flash_dump.bin:r

.PHONY: fuses-boot
fuses-boot:
	$(AVRDUDE_CMD) -Ulock:w:$(LOCKFUSE):m -Uhfuse:w:$(HFUSE_BOOT):m -Ulfuse:w:$(LFUSE):m -Uefuse:w:$(EFUSE):m

.PHONY: fuses-noboot
fuses-noboot:
	$(AVRDUDE_CMD) -Ulock:w:$(LOCKFUSE):m -Uhfuse:w:$(HFUSE_NOBOOT):m -Ulfuse:w:$(LFUSE):m -Uefuse:w:$(EFUSE):m

.PHONY: erase
erase:
	$(AVRDUDE_CMD) -e

.PHONY: target-probe
target-probe:
	$(AVRDUDE_CMD)

canboot-$(BL_VERSION).tar.gz:
	curl -OL https://github.com/sectioncritical/atmega_can_bootloader/releases/download/v$(BL_VERSION)/$@

.PHONY: program-boot
program-boot: canboot-$(BL_VERSION).tar.gz
	tar -zxvf $<
	$(AVRDUDE_CMD) -U flash:w:canboot-$(BL_VERSION)/canboot.hex:i
	rm -rf canboot-$(BL_VERSION)

.PHONY: package
package: $(HEXFILE)
	rm -rf package
	mkdir -p package
	cp $(HEXFILE) package/.
	cp $(ELFFILE) package/.
	cp $(OUT)/$(PROGNAME).map package/.
	cp ../CHANGELOG.md package/.
	PKGNAME=$(PROGNAME)-$(VERSION); rm -rf $$PKGNAME; \
	mv package $$PKGNAME; \
	tar -zcvf $$PKGNAME.tar.gz $$PKGNAME/*; \
	rm -rf $$PKGNAME

# download a release package from github
$(PROGNAME)-$(VERSION).tar.gz:
	curl -OL https://github.com/sectioncritical/zeva24_firmware/releases/download/v$(VERSION)/$@

# extract hex from release package and program it
.PHONY: program-pkg
program-pkg: $(PROGNAME)-$(VERSION).tar.gz
	PKGNAME=$(PROGNAME)-$(VERSION); \
	tar -zxvf $$PKGNAME.tar.gz; \
	$(AVRDUDE_CMD) -U flash:w:$$PKGNAME/$(PROGNAME).hex:i; \
	rm -rf $$PKGNAME

# set up python virtual environment
venv: venv/bin/activate

venv/bin/activate: requirements.txt
	test -d venv || python3 -m venv venv
	. venv/bin/activate; \
	python -m pip install -U pip setuptools wheel; \
	python -m pip install -Ur $<
	touch venv/bin/activate

.PHONY: cleanvenv
cleanvenv:
	rm -rf venv

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

PROGNAME=BMS24

# AVRDUDE ports
# you will need to override these for your system. The easiest thing to do
# is to set an environment variable and then you wont need to type on command
# line each time you run this Makefile.
#
# AVRDUDE_PORT is the device port used for programmer like ArduinoISP
# it is used for any Makefile targets that "program" something. If you are
# not using the programmer, then this does not matter.
#
AVRDUDE_PORT?=/dev/cu.usbmodem14101

# settings for avrdude binary. You can override these to match your system.
# avrdude should be installed somewhere on your system. The default setting
# below assumes it is on your path. Standard avrdude does not include
# support for atmega61m1 so the m16m1.conf file is specified as a supplement.
# You can override the location of avrdude executable and conf file if you
# need to. (the + means add to system configuration)
# TODO: come up with better way to find existing avrdude
#AVRDUDE?=$(shell command -v avrdude)
#AVRDUDE_CONF?= +avrdude/m16m1.conf
AVRDUDE?=~/.platformio/packages/tool-avrdude/bin/avrdude
AVRDUDE_CONF?=~/.platformio/packages/tool-avrdude/avrdude.conf
AVRDUDE_DEVICE=atmega16m1

OUT=zeva_v0

# to run the versioning tool we need to switch around to different
# directories. So it is handy to be able to refer to directopries and files
# with full paths.
BUILD_DIR=$(shell pwd)
PROJ_DIR=$(shell cd ..; pwd)
BUILDLOG=buildlog.txt

# device remains unlocked
LOCKFUSE=0xff

# enable SPI memory access, EESAVE (dont erase eeprom), minimal boot size
HFUSE=0xd7

# disable /8, external osc, longer startup time
LFUSE=0xdf

# BOD level 4.2
EFUSE=0xfa

HEXFILE:=$(OUT)/$(PROGNAME).hex
UPLOADFILE?=$(HEXFILE)

all: help

.PHONY: help
help:
	@echo ""
	@echo "Zeva BMS24 Makefile Help"
	@echo "========================"
	@echo ""
	@echo "Development Host (Building and Programming)"
	@echo "-------------------------------------------"
	@echo "program          - program hex file to target using avrdude/programmer"
	@echo "fuses            - program the fuses using programmer"
	@echo "erase            - erase the flash"
	@echo "dumpflash        = save flash contents to binary file"
	@echo ""
	@echo "Test Host (Testing)"
	@echo "-------------------"
	@echo "probe			- probe bus for devices"
	@echo "monitor          - run monitoring program"
	@echo ""
	@echo "Python Virtual Environment"
	@echo "--------------------------"
	@echo "venv             - create the python virtual environment"
	@echo "cleanvenv        - clean the python virtual environment"
	@echo ""
	@echo "Defaults"
	@echo "--------"
	@echo "AVRDUDE_PORT    = $(AVRDUDE_PORT)"
	@echo ""

# flash the firmware onto the target
.PHONY: program
program: $(HEXFILE)
	$(AVRDUDE) -P$(AVRDUDE_PORT) -b19200 -v -p $(AVRDUDE_DEVICE) -C $(AVRDUDE_CONF) -c stk500v1 -U flash:w:$<:i

# dump the flash contents into a file
.PHONY: dumpflash
dumpflash:
	$(AVRDUDE) -P$(AVRDUDE_PORT) -b19200 -v -p $(AVRDUDE_DEVICE) -C $(AVRDUDE_CONF) -c stk500v1 -U flash:r:flash_dump.bin:r

.PHONY: fuses
fuses:
	$(AVRDUDE) -P$(AVRDUDE_PORT) -b19200 -v -p $(AVRDUDE_DEVICE) -C $(AVRDUDE_CONF) -c stk500v1 -Ulock:w:$(LOCKFUSE):m -Uhfuse:w:$(HFUSE):m -Ulfuse:w:$(LFUSE):m -Uefuse:w:$(EFUSE):m

.PHONY: erase
erase:
	$(AVRDUDE) -P$(AVRDUDE_PORT) -b19200 -v -p attiny841 -C $(AVRDUDE_CONF) -c stk500v1 -e

.PHONY: probe
probe: venv
	venv/bin/bmsutil -d $(LOAD_PORT) -b $(BAUD) discover

.PHONY: monitor
monitor: venv
	venv/bin/bmsutil -d $(LOAD_PORT) -b $(BAUD) monitor

# system should have python3 available
PY3?=$(shell command -v python3)

# set up python virtual environment
venv: venv/bin/activate

venv/bin/activate: requirements.txt
	test -d venv || $(PY3) -m venv venv
	. venv/bin/activate; pip install -U pip setuptools wheel; pip install -Ur $<
	touch venv/bin/activate

.PHONY: cleanvenv
cleanvenv:
	rm -rf venv


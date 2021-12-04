Building and Programming the Firmware
=====================================

Requirements
------------

### Building

This project uses a Makefile to build and program the firmware. It assumes the
presence of a typical posix-like devlopment environment: bash-like shell,
development tools like make, etc.

It uses the *AVR GCC toolchain*. There is information about the toolchain and
the library at the [AVR Libc Home Page](https://www.nongnu.org/avr-libc/). This
page also has a link to the WinAVR project which provides a toolchain version
for Windows (not tested by me).

The toolchain is also available from various package managers (try "gcc-avr" or
"avr-gcc").

It is also available
[directly from Microchip](https://www.microchip.com/en-us/development-tools-tools-and-software/gcc-compilers-avr-and-arm).

### Programming to Flash

[AVRDUDE](https://www.nongnu.org/avrdude/) is used for loading the firmware
into the flash memory of the microcontroller. This is also available through
package managers. However I found the version available through MacPorts to
not be reliable. Instead I installed it using [PlatformIO](https://platformio.org)
with the Arduino framework which automatically gets you avrdude.

### Uploading via Boot Loader

**Not yet implemented** - The firmware can be uploaded to the target MCU via
the CAN boot loader. This requires a certain hardware configuration. It also
requires Python3.

Hardware
--------

This is meant to run on a ZEVA BMS24 board. It has an ATMega16M1 running at
8 MHz. It uses the SPI interface for programming with a
[USBTiny](https://www.adafruit.com/product/46) programming dongle. It could be
made to work with other AVR programmers.

More links to the original ZEVA project are listed in the
[top level README](../README.md). And here is a related
[circuit board project](https://github.com/sectioncritical/zeva24_board).

Here is some more
[information about the hardware](https://github.com/sectioncritical/zeva24_firmware/blob/main/docs/dev_hardware.md).

Usage
-----

### Development Flow Without Boot Loader

For normal development, code is built and burned into flash. A boot loader is
not used here. After programming the flash, the BMS programs starts running.

    make clean      # optional
    make            # build firmware, create hex file
    make program    # programs hex file into target flash memory

If the MCU already had a boot loader installed, it will be erased by the above
steps.

#### First time Fuse setup

If the MCU has never been programmed, the fuses need to be set. This only needs
to be done once:

    make fuses-noboot

(see below for boot loader fuse settings)

### Boot Loader

To install the boot loader on a board that is ready to be deployed, the fuses
need to bet set (only once):

    make fuses-boot

This will download the boot loader from GitHub (uses `curl`) and program it to
flash memory:

    make program-boot

If you want to specify a boot loader release version:

    make program-boot BL_VERSION=1.2.3

At the time of this writing, there is only one boot loader version, 1.0.0.

**Notes about boot loader:** once the boot loader is installed, you need to use
it to install the BMS firmware, instead of the programmer method above. This is
because the boot loader calculates a CRC for the program as it is uploader, and
the boot loader will not allow the program to start if the CRC does not match.
Using `make program` bypasses the CRC calculation so it will not have a valid
CRC and the boot loader will not allow the BMS program to start.

In the future I may add a new Makefile target for uploading using the CAN
boot loader. In the meantime, you can take a look at the
[canloader python utility](https://github.com/sectioncritical/atmega_can_bootloader/tree/main/util).

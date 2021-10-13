ZEVA BMS24 Firmware Package
===========================

This is the **ZEVA 24-cell Lithium BMS Module** firmware. See this link for
the product page (working at the time of this writing):

https://www.zeva.com.au/index.php?product=143

ZEVA was a company that makes electric vehicle components such as battery
management systems. In August 2021 they converted their product to open source.
See the notice here (ZEVA home page working at the time of this writing):

https://www.zeva.com.au

Here is the link for the open source package of the BMS24 package. This is
where I downloaded the original design documents package for conversion to this
repo:

https://www.zeva.com.au/Products/designFiles/ZEVA_BMS24.zip

The design documents were released under the [MIT License](https://choosealicense.com/licenses/mit/).
Refer to the [LICENSE file](LICENSE.txt).

* * * * *

This repository currently holds the original firmware source code and hex file
from the Zeva design files package. It provides a place to modify the firmware
to add additional features. In the future it will allow rebuilding the firmware
with a Makefile.

The original firmware source code and hex file are located in
[build/zeva_v0](build/zeva_v0).

### Board Project

This is the circuit board for this project:

https://github.com/sectioncritical/zeva24_board

Hardware Setup
--------------

See [this document](docs/dev_hardware.md) for more information about the
hardware configuration.

Programming
-----------

There is a Makefile to help with programming tasks. The two main targets are:

    make fuses
    make program

This will program the fuses, and then load the factory firmware. It makes use
of `avrdude` which must be installed on your system. Set the `AVRDUDE`
environment variable to point to the `avrdude` binary.

This use of `avrdude` for programming also assumes you have a programming
dongle connected to the BMS board via the 5-pin programming connector. I used
an inexpensive ATMEGA32U4-based Arduino-like board, using the "Arduino as ISP"
firmware. I used jumper wires to connect the programmer board to the BMS target
board.

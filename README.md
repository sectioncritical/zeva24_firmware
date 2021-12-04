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

This project continues to use the same license.

* * * * *

This project is firmware for the above-mentioned ZEVA BMS24 board. It starts
with the original firmare released by ZEVA, but with modifications over time.
See the commit history to see the historical changes.

Features
--------

No new features have yet been added. The original firmware has a simple CAN
protocol that allows you to read the voltage of each cell as well as 4
temperature sensors. You can set the voltage threshold for shunting.

Future improvements may include:

- fix warnings in original code
- add command to enter boot loader. This will allow in-system firmware update
- add readable firmware version
- modify code to reduce power when not in active use

There may also be future modifications to the original board design. This
project will track board changes with firmware updates as needed.

Usage
-----

When the BMS firmware is installed on a board and the board is deployed into
a battery system, you can read the cell voltages via a CAN bus protocol. The
[Monitor Utility](https://github.com/sectioncritical/zeva12can) is a simple
Python program that will read the voltages.

Building and Programming
------------------------

See the [build directory](build).

**Note:** The first commit in the `src/` directory is the original, unmodified
source code from ZEVA.

Hardware Setup
--------------

See [this document](docs/dev_hardware.md) for more information about the
hardware configuration.

Related Projects
----------------

This project was created as a result of ZEVA making their products open-source.
Other project related to this are:

- [CAN Boot Loader](https://github.com/sectioncritical/atmega_can_bootloader)
- [ZEVA BMS24 Board](https://github.com/sectioncritical/zeva24_board)
- [Monitor Utility](https://github.com/sectioncritical/zeva12can)

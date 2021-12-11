[![](doc/img/license-badge.svg)](https://opensource.org/licenses/MIT)
[![build-bootloader](https://github.com/sectioncritical/zeva24_firmware/actions/workflows/build.yml/badge.svg)](https://github.com/sectioncritical/zeva24_firmware/actions/workflows/build.yml)


The authoritative location for this project is:
https://github.com/sectioncritical/zeva24_firmware

* * * * *

**NOT SUPPORTED BY ZEVA**

This project, and the owner of this repo, have no association with ZEVA. I made
this project based on the design files released to open source by ZEVA but they
have nothing to do with this repository. **Please do not ask ZEVA for help with
this project.**

* * * * *

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

Versioning
----------

The original functionality is versioned and released in this repository as
[`1.0.0`](https://github.com/sectioncritical/zeva24_firmware/releases/tag/v1.0.0).
All changes after that will be versioned using [semantic versioning](semver.org),
with minor versions (ex: 1.1.0) for new features that remain compatible with
the original protocol, and major versions (ex: 2.0.0) when it is no longer
compatible with the original.

Features
--------

The original firmware has a simple CAN protocol that allows you to read the
voltage of each cell as well as 4 temperature sensors. You can set the voltage
threshold for shunting.

See the [CHANGELOG](CHANGELOG.md) for a list of changes since the original
release.

There may also be future modifications to the original board design. This
project will track board changes with firmware updates as needed.

Usage
-----

Here is the [BMS CAN Protocol](doc/protocol.md).

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

See [this document](doc/dev_hardware.md) for more information about the
hardware configuration.

Related Projects
----------------

This project was created as a result of ZEVA making their products open-source.
Other project related to this are:

- [CAN Boot Loader](https://github.com/sectioncritical/atmega_can_bootloader)
- [ZEVA BMS24 Board](https://github.com/sectioncritical/zeva24_board)
- [Monitor Utility](https://github.com/sectioncritical/zeva12can)

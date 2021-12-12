CHANGELOG
=========

BMS Firmware for the ZEVA BMS24 board.

Visit the repository for documentation.

**Repository:** https://github.com/sectioncritical/zeva24_firmware

[Keep a Changelog](https://keepachangelog.com/en/1.0.0/) /
[Semantic Versioning](https://semver.org/spec/v2.0.0.html)

**NOTE:** This project is not associated with ZEVA and they are not responsible
for anything here.

## [1.1.0] - 2021-12-12

### Bug Fixes

- Fix compile warnings and turn -Werror back on ...

### Features

- Add version and reboot commands

### Testing

- Add bloaty code size checker ...

### Documentation

- Add CAN protocol doc

### Styling

- Removed tabs. large diff of whitespace
- Add cppcheck and fix warnings from it
- Added MISRA checker and address all issues as a result ...


## [1.0.0] - 2021-12-04

Initial release of the BMS firmware for this project. The source code is the
unmodified source code from ZEVA that was released to open-source in 2021.
The purpose of this project is to provide a controlled starting point for
future enhancements.

### Features

- runs on ZEVA BMS24 board
- original feature set
- provides cell voltages over CAN bus in response to a query, per the ZEVA
  BMS12 CAN protocol
- provides 4 temperature sensor readings as well

* * * * *

[1.0.0]: https://github.com/sectioncritical/zeva24_firmware/releases/tag/v1.0.0
[1.1.0]: https://github.com/sectioncritical/zeva24_firmware/releases/tag/v1.1.0

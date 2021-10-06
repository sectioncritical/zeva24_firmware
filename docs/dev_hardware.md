Development Hardware Setup
==========================

This documents the hardware setup for development and debug of the Zeva
24-channel BMS. Here is the high level block diagram.

```
+-------+   SPI     +---------------+
| RPi   *-----------* MCP2515-based |
+---*---+           |   CAN Board   |
                    +-------*-------+
                            |
                            | CAN
                            |
+-----------+   DBG     +---*---+ 12V   +-------+
|  Arduino  *-----------* Zeva  *-------* PWR   |
|    ISP    |           | BMS24 |       +-------+
+---*-------+           +-------+
    |
    | USB
    |
+---*---+
| Dev   |
| Host  |
+-------+
```

RPi/CAN Board
-------------

The Raspberry Pi is used as a test fixture for running test code and CAN comms.
Currently it is a RPi 3 B+ but it probably doesnt matter. The RPi is set up
with Raspbian lite (headless) and the normal suite of development packages.
In addition it has:

* can-utils

The CAN board is based on MCP2515 CAN controller. There are many boards
available that use this device. Currently we are using this:

https://www.mikroe.com/can-spi-33v-click

This requires using jumper wires to connect the CAN board to the RPi. In the
future it would be better to get a Pi Hat with CAN, or design a custom test
fixture Hat that includes the CAN controller.

Jumper wire setup:

| Pi | Board       |
|----|-------------|
| 6  | GND         |
| 1  | 33V         |
| 19 | SDI (MOSI)  |
| 21 | SDO (MISO)  |
| 23 | SCK         |
| 24 | CS          |
| 22 | INT (GPIO25)|

There are multiple options for GND and 3.3V. Also multiple choices of GPIO can
be used for INT. It is also possible to use SPI1 but I didn't try that.

### RPi Config for CAN

The RPi requires several one-time configuration steps.

#### /boot/config.txt

This file needs to be edited using `sudo` or as root. Add the following lines
as needed:

```
dtparam=spi=on
dtoverlay=mcp2515-can0,oscillator=10000000,interrupt=25
```

Make sure that `oscillator` value matches whatever is on your CAN board. Also,
The `interrupt` parameter is the RPi GPIO number where you connected the CAN
board INT pin.

This will not take effect until you reboot the RPi.

#### /etc/modprobe.d/raspi-blacklist.conf

I am not sure if this is really needed. I saw one site mention this was
required, but didnt see it mentioned elsewhere. Add the following to this file:

```
spi-bcm2708
```

#### Set up CAN socket

    sudo ip link set can0 up type can bitrate 250000

This can be automated to a script, or the RPi can be setup to do this at boot
time.

#### /etc/network/interfaces

*Not tried yet*

```
auto can0
iface can0 inet manual
    pre-up /sbin/ip link set can0 type can bitrate 250000 triple-sampling on restart-ms 100
    up /sbin/ifconfig can0 up
    down /sbin/ifconfig can0 dow
```

Note: in Bill's config, he did not have `triple-sampling` and had default `off`
instead of `on`. Also, he had `listen-only`.

Zeva BMS24 Board
----------------

Rough block diagram, to show relevant connector pinouts:

```
                          SCK   MOSI
                       RST   MISO  GND
        +---------------*--*--*--*--*---------------+
        |               5  4  3  2  1               |
 SHIELD *                   DEBUG                   * 12V
    GND *                                           * CAN-H
  CAN-L *                                           * CAN-L
  CAN-H *                                           * GND
    12V *                                           * SHIELD
        |   +---------------+   +---------------+   |
        |   |    CELLS-H    |   |   CELLS-L     |   |
        +---+---------------+---+-------------------+
```

Notes: temperature sensor connectors are not shown. Cell connector details are
not shown.

The connectors on the left and right are for power and CAN bus. They are
identical except that the top-bottom pin order is reversed. This allows the
cable connectors to always be wired the same way and not matter which plug is
used.

The debug connector is for the AVR SPI-based debug port.

### Debugger

The "debugger" is not really a debugger but a memory loader. It uses an
Arduino-like board with "Arduino as ISP" firmware loaded. This allows it to
be plugged into the USB host, and the SPI port connected to the
unit-under-test. Then, `avrdude` is used on the host to load the target flash.

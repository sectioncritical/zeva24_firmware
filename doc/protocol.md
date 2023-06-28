BMS CAN Protocol
================

This document is a reformatting of the original
[ZEVA BMS12 CAN Protocol](https://www.zeva.com.au/Products/datasheets/BMS12v3_CAN_Protocol.pdf)
document. It contains the original protocol messages along with any new
messages that are added for new features.

CAN Parameters
--------------

By default, unless changed in the firmware, this protocol uses a data rate of
250 kbits. The message format uses 29-bit message IDs.

Message IDs
-----------

In the original protocol, there is one message from controller to BMS devices,
and 4 reply messages from BMS device to controller. The CAN message IDs for
each message use a base ID plus a message type.

The "Command" and "Response"  message were added in `1.1.0`.

### Base ID

Each BMS device has an ID assigned using the rotary switch (or possible some
other method). The device ID is 0-15 (decimal). These are used to form a base
CAN message ID as following (all numbers are decimal):

    CAN Message ID = 300 + (BMS_ID * 10)

The BMS ID to CAN Message ID table is shown below:

|BMS ID |Base CAN  ID|
|-------|------------|
|   0   | 300        |
|   1   | 310        |
|  ...  | ...        |
|   9   | 390        |
|  10   | 400        |
|  ...  | ...        |
|  15   | 450        |

### CAN Message ID

The CAN message ID is the Base ID plus the message type. For example the
*Reply1* message is type 1. For BMS device 2, the CAN message ID would
be 321:

    300 + ((BMS unit 2) * 10) + (msg type 1)
    300 + (     2       * 10) +      1        ==> 321

### BMS24 vs BMS12

This protocol was originally for the BMS12 with 12-cells and 2 temperature
sensors. For the purposes of CAN protocol, a BMS24 appears like 2 BMS12
devices. Each one presents as 2 BMS units on the CAN bus. If the rotary switch
is set to 0, then the BMS24 presents as units 0 and 1.

Messages
--------

Note: Cell voltages are numbered using 1-origin. So 12 cell voltages are
numbered 1-12.

|Message| Type  |Description                        |
|-------|-------|-----------------------------------|
|Request|   0   |Controller request data from device|
|Reply1 |   1   |Cell voltage 1-4                   |
|Reply2 |   2   |Cell voltage 5-8                   |
|Reply3 |   3   |Cell voltage 9-12                  |
|Reply4 |   4   |Temperature 1 and 2                |
|Command|   5   |Command message (data dependent)   |
|Response|  6   |Response to command (data dependent)|

* * * * *

### Request (0)

|Message ID|Length|
|----------|------|
| Base + 0 |  2   |

#### Message Data

| Byte  | Meaning                   |
|-------|---------------------------|
| 0     | shunt voltage high byte   |
| 1     | shunt voltage low byte    |

#### Description

This message is sent from the controller to a BMS device. It serves two
purposes: it sets the BMS shunt voltage, and it requests data from the BMS.
The shunt voltage is a 16-bit value in millivolts, big-endian format in the
CAN data field.

If the shunt voltage is 0, the shunt balancer is disabled. This message must
be sent at least once per second for the shunt balancer to remain enabled.

This message also causes the BMS to transmit cell voltages and temperatures via
the *Reply1-Reply4* messages.

* * * * *

### Reply1 (1)

|Message ID|Length|
|----------|------|
| Base + 1 |  8   |

#### Message Data

| Byte  | Meaning                   |
|-------|---------------------------|
| 0     | Cell voltage 1 high byte  |
| 1     | Cell voltage 1 low byte   |
| 0     | Cell voltage 2 high byte  |
| 1     | Cell voltage 2 low byte   |
| 0     | Cell voltage 3 high byte  |
| 1     | Cell voltage 3 low byte   |
| 0     | Cell voltage 4 high byte  |
| 1     | Cell voltage 4 low byte   |

#### Description

This message is sent in response to a *Request* message. It contains cell
voltages 1-4 as 16-bit values in millivolts. The 16-bit values are stored in
big endian format in the message data field.

* * * * *

### Reply2 (2)

|Message ID|Length|
|----------|------|
| Base + 2 |  8   |

#### Message Data

| Byte  | Meaning                   |
|-------|---------------------------|
| 0     | Cell voltage 5 high byte  |
| 1     | Cell voltage 5 low byte   |
| 0     | Cell voltage 6 high byte  |
| 1     | Cell voltage 6 low byte   |
| 0     | Cell voltage 7 high byte  |
| 1     | Cell voltage 7 low byte   |
| 0     | Cell voltage 8 high byte  |
| 1     | Cell voltage 8 low byte   |

#### Description

This message is sent immediately following a *Reply1* message, in response to
a *Request* message. It contains cell voltages 5-8 as 16-bit values in
millivolts. The 16-bit values are stored in big endian format in the message
data field.

* * * * *

### Reply3 (3)

|Message ID|Length|
|----------|------|
| Base + 3 |  8   |

#### Message Data

| Byte  | Meaning                   |
|-------|---------------------------|
| 0     | Cell voltage 9 high byte  |
| 1     | Cell voltage 9 low byte   |
| 0     | Cell voltage 10 high byte |
| 1     | Cell voltage 10 low byte  |
| 0     | Cell voltage 11 high byte |
| 1     | Cell voltage 11 low byte  |
| 0     | Cell voltage 12 high byte |
| 1     | Cell voltage 12 low byte  |

#### Description

This message is sent immediately following a *Reply2* message, in response to
a *Request* message. It contains cell voltages 9-12 as 16-bit values in
millivolts. The 16-bit values are stored in big endian format in the message
data field.

* * * * *

### Reply4 (4)

|Message ID|Length|
|----------|------|
| Base + 4 |  8   |

#### Message Data

| Byte  | Meaning       |
|-------|---------------|
| 0     | Temperature 1 |
| 1     | Temperature 2 |
| 2:7   | reserved (0)  |

#### Description

This message is sent immediately following a *Reply3* message, in response to
a *Request* message. It contains two temperature readings from the temperature
sensors. The temperatures are 8-bit values, in units of degrees C with a 40C
offset. To get temperature in C:

    temp_in_C = temp_data - 40

**NOTE:** The PDF protocol document from ZEVA states this message has 2 bytes.
But the ZEAV code is written to send 8 bytes with unused bytes as zeroes.

* * * * *

### Command (5)

|Message ID|Length|
|----------|------|
| Base + 5 |  8   |

#### Version Notes

|Version|Notes                                                      |
|-------|-----------------------------------------------------------|
| `1.1` |message introduced with version and reboot functions       |

#### Message Data

| Byte  | Meaning       |
|-------|---------------|
| 0     | Command type  |
| 1:7   | Reserved(0)   |

#### Description

This message is a command from the controller to a BMS device. A single message
with a command type in the data field was chosen to save message ID space, and
to reduce the future need for additional messages. This approach allows adding
more commands in the future without adding more message types, and keeping the
protocol backwards compatible.

#### Commands

|Command Type   |Data   |Meaning    |
|---------------|-------|-----------|
| 0             | 0     |Reboot     |
| 1             | 0     |Version    |

##### Reboot Command

This command initiates a reboot in the BMS device. If a boot loader is
installed, the BMS will reboot into the boot loader, and the boot loader
protocol can be used to load new firmware.

Prior to rebooting, the BMS device will send a Response to acknowledge the
Reboot command.

##### Version Command

This command is used to report the firmware version back to the controller.
The BMS device will send a Response message containing the version.

* * * * *

### Response (6)

|Message ID|Length|
|----------|------|
| Base + 6 |  8   |

#### Version Notes

|Version|Notes                                                      |
|-------|-----------------------------------------------------------|
| `1.1` |message introduced with version and reboot functions       |

#### Message Data

| Byte  | Meaning                   |
|-------|---------------------------|
| 0     | Response type             |
| 1:7   | Dependent on response type|

#### Description

This message is a response by the BMS device to a Command message from the
controller.

#### Responses

|Response  Type |Data   |Meaning            |
|---------------|-------|-------------------|
| 0             | 0     |Reboot Acknowledge |
| 1             | +3    |Firmware version   |

##### Reboot Response

This response is an acknowledgement of a Reboot command  and is sent by the
BMS device just before rebooting itself.

##### Version Response

This is a response to a Version command and contains the firmware version in
3 bytes.

This command is used to report the firmware version back to the controller.
The BMS device will send a Response message containing the version. The version
will be 3 additional bytes in the data field.

| Byte  | Meaning                   |
|-------|---------------------------|
| 0     | Response type             |
| 1     | Major version             |
| 2     | Minor version             |
| 3     | Patch version             |
| 4:7   | Reserved(0)               |

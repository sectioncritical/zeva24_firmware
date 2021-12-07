// BMS24: 24-cell Lithium Battery Management Module
// Open Source version, released under MIT License (see Readme file)
// Last modified by Ian Hooper (ZEVA), August 2021

// Code for ATmega16M1 (also suitable for ATmega32M1, ATmega64m1)
// Fuses: 8Mhz+ external crystal, CKDIV8 off, brownout 4.2V

#define CAN_BAUD_RATE   250 // Code knows how to do 125, 250, 500, 1000kbps
#define USE_29BIT_IDS   1u   // Or 0 for 11-bit IDs
#define DISABLE_TEMPS   0

#define LOW_LTC_CORRECTION      6u   // Calibration to account for voltage drop through buffer resistors and LTC6802 variations
#define HIGH_LTC_CORRECTION     6u   // Typical value is about 6 for both of these, but may be +/-3mV in some cases

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/wdt.h>

#define BASE_ID 300U // Starting ID used for BMS module messaging to/from EVMS
// cppcheck-suppress [misra-c2012-2.4] checker is confused here
enum { BMS12_REQUEST_DATA, BMS12_REPLY1, BMS12_REPLY2, BMS12_REPLY3, BMS12_REPLY4 };

#define COMMS_TIMEOUT   32u // at 32Hz, i.e 1 second timeout

// SPI interface pins
#define CSBI        (1<<PC5)
#define CSBI_PORT   PORTC
#define SDO         (1<<PC4)
#define SDO_PORT    PINC
#define SDI         (1<<PC6)
#define SDI_PORT    PORTC
#define SCKI        (1<<PB3)
#define SCKI_PORT   PORTB

#define CSBI2       (1<<PB6)
#define CSBI2_PORT  PORTB
#define SDO2        (1<<PB5)
#define SDO2_PORT   PINB
#define SDI2        (1<<PB7)
#define SDI2_PORT   PORTB
#define SCKI2       (1<<PD0)
#define SCKI2_PORT  PORTD

// Status LED
#define GREEN_PORT  PORTC
#define GREEN       (1<<PC1)
#define RED_PORT    PORTD
#define RED         (1<<PD3)

// LTC6802 Command codes
#define WRCFG   0x01
//#define RDCFG   0x02
#define RDCV    0x04
#define RDTMP   0x08
#define STCVAD  0x10
#define STTMPAD 0x30

// Inputs from hex rotary pot for module ID selection
#define MOD_ID_NUM8     (PIND & (1<<PD5))
#define MOD_ID_NUM4     (PIND & (1<<PC7))
#define MOD_ID_NUM2     (PINB & (1<<PB2))
#define MOD_ID_NUM1     (PIND & (1<<PD6))

// Function declarations
static void SetupPorts(void);
static int LineariseTemp(uint16_t adc);
static void CanTX(uint32_t packetID, uint8_t bytes);
static void WriteSPIByte(uint8_t byte);
static uint8_t ReadSPIByte(void);
static void WriteSPIByte2(uint8_t byte);
static uint8_t ReadSPIByte2(void);
static void GetModuleID(void);

// Global variables
static uint8_t txData[8]; // CAN transmit buffer
static volatile bool dataRequestedL = false; // Low group, LTC #2
static volatile bool dataRequestedH = false; // High group, LTC #1

static uint16_t moduleID = 0;

static volatile uint16_t shuntVoltage; // In millivolts

// cppcheck-suppress [misra-c2012-2.7,misra-c2012-8.2,misra-c2012-8.4]
ISR(CAN_INT_vect) // Interrupt function when a new CAN message is received
{
    int8_t savedCANPage;
    savedCANPage = CANPAGE; // Saves current MOB
    CANPAGE = CANHPMOB & 0xF0; // Selects MOB with highest priority interrupt
    if ((CANSTMOB & (1 << RXOK)) != 0)
    {
        uint8_t rxData[8];
        int8_t length;
        length = CANCDMOB & 0x0F; // Number of bytes to receive is bottom four bits of this reg
        for (int8_t i = 0; i < length; i++)
        {
            rxData[i] = CANMSG; // This autoincrements when read
        }

        // ID has top 8 bits in IDT1 and bottom 3 bits at TOP of IDT2
        uint32_t rxPacketID;
        if (USE_29BIT_IDS != 0u)
        {
            rxPacketID = (((uint32_t)CANIDT1) << 21)
                       + (((uint32_t)CANIDT2) << 13)
                       + (CANIDT3 << 5) + (CANIDT4 >> 3);
        }
        else
        {
            rxPacketID = (CANIDT1 << 3) + (CANIDT2 >> 5);
        }

        // Only one packet we care about - the data request
        if (rxPacketID == moduleID) // Request is for us
        {
            shuntVoltage = (rxData[0]<<8) + rxData[1]; // Big endian format (high byte first)
            dataRequestedL = true;
        }

        if (rxPacketID == (moduleID + 10u)) // Or the next ID
        {
            shuntVoltage = (rxData[0] << 8) + rxData[1];
            dataRequestedH = true;
        }
        // Enable reception, data length 8
        CANCDMOB = (1u << CONMOB1) | (8u << DLC0) | ((1u << IDE) * USE_29BIT_IDS);
        // Note: The DLC field of CANCDMOB register is updated by the received MOB, and if it differs from above, an error is set
    }
    CANSTMOB = 0x00; // Reset interrupt reason on selected channel
    CANPAGE = savedCANPage;
}

int main(void)
{
    static uint16_t voltage[24]; // In millivolts
    static int16_t temp[4]; // In deg C
    uint32_t shuntBits = 0;
    uint8_t commsTimer = 0;

    _delay_ms(100); // Allow everything to stabilise on startup

    SetupPorts();

    GetModuleID();

    // Initialising variables
    (void)memset(voltage, 0, sizeof(voltage));
    (void)memset(temp, 0, sizeof(temp));

    sei(); // Enable interrupts
    wdt_enable(WDTO_120MS); // Enable watchdog timer

    uint8_t cellBytes[2][25];
    uint8_t tempBytes[2][5];
    uint16_t voltages[24][8];
    uint16_t tempBuffer[4][8];

    uint8_t counter = 0;
    uint8_t slowCounter = 0;
    while (1)
    {
        wdt_reset();

        // Comms with LTC6802s..
        // Split up the 32-bit shuntBits variable into two 12-bit chunks for each LTC
        uint32_t shuntBitsL = shuntBits & 0x0FFFu; // Lower 12 bits
        uint32_t shuntBitsH = shuntBits >> 12; // Upper 12 bits

        // Write config registers, LTC #1, which is the left side (more positive)
        CSBI_PORT &= ~CSBI; // Pull down to start command
        WriteSPIByte(WRCFG); // write configuration group
        WriteSPIByte(0b00000001);
        WriteSPIByte(((unsigned short)(shuntBitsH & 0x00FFu))); // Bottom byte of shunt bits
        WriteSPIByte(((unsigned short)(shuntBitsH >> 8))); // Top four bits of shunt bits, right shifted 1 byte
        WriteSPIByte(0b00000000);
        WriteSPIByte(0b00000000);
        WriteSPIByte(0b00000000);
        CSBI_PORT |= CSBI; // Pull up to end command
        _delay_us(100);

        // Start voltage sampling
        CSBI_PORT &= ~CSBI;
        WriteSPIByte(STCVAD);
        CSBI_PORT |= CSBI;

        // Write config registers, LTC #2, which is the right side (more negative)
        CSBI2_PORT &= ~CSBI2; // Pull down to start command
        WriteSPIByte2(WRCFG); // write configuration group
        WriteSPIByte2(0b00000001);
        WriteSPIByte2(((uint16_t)(shuntBitsL & 0x00FFu))); // Bottom byte of shunt bits
        WriteSPIByte2(((uint16_t)(shuntBitsL >> 8))); // Top four bits of shunt bits, right shifted 1 byte
        WriteSPIByte2(0b00000000);
        WriteSPIByte2(0b00000000);
        WriteSPIByte2(0b00000000);
        CSBI2_PORT |= CSBI2; // Pull up to end command
        _delay_us(100);

        // Start voltage sampling
        CSBI2_PORT &= ~CSBI2;
        WriteSPIByte2(STCVAD);
        CSBI2_PORT |= CSBI2;

        _delay_ms(20); // Cell sampling can take up to 16ms - anything we need to do in the meantime? Not really.

        // Start temperature sampling
        CSBI_PORT &= ~CSBI;
        WriteSPIByte(STTMPAD);
        CSBI_PORT |= CSBI;

        // Start temperature sampling, ltc #2
        CSBI2_PORT &= ~CSBI2;
        WriteSPIByte2(STTMPAD);
        CSBI2_PORT |= CSBI2;

        _delay_ms(5); // Temp sampling should only take ~3ms

        // Read cell voltage registers
        CSBI_PORT &= ~CSBI;
        WriteSPIByte(RDCV);
        SDI_PORT |= SDI;
        for (uint8_t n = 0; n < 25u; n++)
        {
            cellBytes[0][n] = ReadSPIByte();
        }
        CSBI_PORT |= CSBI;
        _delay_us(100);

        // Read temperature data
        CSBI_PORT &= ~CSBI;
        WriteSPIByte(RDTMP);
        SDI_PORT |= SDI;
        for (uint8_t n = 0; n < 5u; n++)
        {
            tempBytes[0][n] = ReadSPIByte();
        }
        CSBI_PORT |= CSBI;
        _delay_us(100);

        // Read cell voltage registers, ltc #2
        CSBI2_PORT &= ~CSBI2;
        WriteSPIByte2(RDCV);
        SDI2_PORT |= SDI2;
        for (uint8_t n = 0; n < 25u; n++)
        {
            cellBytes[1][n] = ReadSPIByte2();
        }
        CSBI2_PORT |= CSBI2;
        _delay_us(100);

        // Read temperature data, ltc #2
        CSBI2_PORT &= ~CSBI2;
        WriteSPIByte2(RDTMP);
        SDI2_PORT |= SDI2;
        for (uint8_t n = 0; n < 5u; n++)
        {
            tempBytes[1][n] = ReadSPIByte2();
        }
        CSBI2_PORT |= CSBI2;
        _delay_us(100);

        // Extract voltage data
        for (uint8_t n = 0; n < 12u; n += 2u)
        {
            uint16_t v = cellBytes[1][(n * 3u) / 2u]; // lower byte
            v += (cellBytes[1][((n * 3u) / 2u) + 1u] & 0x0Fu) << 8; // upper 4 bits
            v = (v * 3u) / 2u; // mV conversion
            voltages[n][counter] = v;
            v = (cellBytes[1][((n * 3u) / 2u) + 1u]) >> 4; // lower 4 bits of next cell
            v += cellBytes[1][((n * 3u) / 2u) + 2u] << 4;  // upper 8 bits
            v = (v * 3u) / 2u;
            voltages[n + 1u][counter] = v;
        }
        for (uint8_t n = 0; n < 12u; n += 2u)
        {
            uint16_t v = cellBytes[0][(n * 3u) / 2u]; // lower byte
            v += (cellBytes[0][((n * 3u) / 2u) + 1u] & 0x0Fu) << 8; // upper 4 bits
            v = (v * 3u) / 2u; // mV conversion
            voltages[n + 12u][counter] = v;
            v = (cellBytes[0][((n * 3u) / 2u) + 1u]) >> 4; // lower 4 bits of next cell
            v += cellBytes[0][((n * 3u) / 2u) + 2u] << 4;  // upper 8 bits
            v = (v * 3u) / 2u;
            voltages[n + 12u + 1u][counter] = v;
        }

        // Extract temperature data
        tempBuffer[0][counter] = tempBytes[1][0] + (256u * (tempBytes[1][1] & 0x0Fu)); // gives mV
        tempBuffer[1][counter] = ((uint8_t)(tempBytes[1][1] & 0xF0u) >> 4)
                               + (tempBytes[1][2] * 16u); // gives mV
        tempBuffer[2][counter] = tempBytes[0][0] + (256u * (tempBytes[0][1] & 0x0Fu)); // gives mV
        tempBuffer[3][counter] = ((tempBytes[0][1] & 0xF0u) >> 4) + (tempBytes[0][2] * 16u); // gives mV

        counter++;
        if (counter >= 8u) // Slow loop, about 4Hz
        {
            counter = 0;

            slowCounter++;
            if (slowCounter >= 4u)
            {
                slowCounter = 0;
            }

            bool notAllZeroVolts = false;
            for (uint8_t n = 0; n < 24u; n++) // Calculate average voltage over last 8 samples, and update shunts if required
            {
                uint16_t average = 0;
                for (uint8_t c = 0; c < 8u; c++)
                {
                    average += voltages[n][c] / 2u;
                }
                voltage[n] = average >> 2;

                uint16_t correction = LOW_LTC_CORRECTION;
                if (n >= 12u)
                {
                    correction = HIGH_LTC_CORRECTION;
                }
                if (voltage[n] > 0u)
                {
                    voltage[n] += correction;
                    if ((n == 0u) || (n == 12u))
                    {
                        voltage[n] -= correction / 2u; // First cells have less drop due to single 3.3Kohm resistor in play
                    }
                }

                if (voltage[n] > 5000u) // Probably means no cells are plugged in to power the LTC
                {
                    voltage[n] = 0;
                }

                if (voltage[n] > 0u)
                {
                    notAllZeroVolts = true;
                }

                if ((voltage[n] > shuntVoltage) && (shuntVoltage > 0u))
                {
                    shuntBits |= (1UL << n);
                }
                else
                {
                    shuntBits &= ~(1UL << n);
                }
            }

            // Calculate temperature averages
            for (uint8_t n = 0; n < 4u; n++)
            {
                uint16_t average = 0;
                for (uint8_t c = 0; c < 8u; c++)
                {
                    average += tempBuffer[n][c];
                }
                temp[n] = average >> 3;
            }

            // Update Status LED(s)
            RED_PORT &= ~RED; // Most cases have red light off and green on
            GREEN_PORT |= GREEN;
            if ((shuntBits != 0u) && (slowCounter & 0x01u)) // Red/orange flash if shunting
            {
                RED_PORT |= RED;
            }
            else if (!notAllZeroVolts) // Blink red if no cells detected
            {
                GREEN_PORT &= ~GREEN;
                if ((slowCounter & 0x01u) != 0u)
                {
                    RED_PORT |= RED;
                }
            }
            // Blink green if no CAN comms
            else if ((commsTimer == COMMS_TIMEOUT) && (slowCounter & 0x01u))
            {
                GREEN_PORT &= ~GREEN;
            }
            else
            {} // there is comms so it will stay green
        }

        if (commsTimer < COMMS_TIMEOUT)
        {
            commsTimer++;
        }
        else
        {
            shuntVoltage = 0; // If comms times out, kill all shunt balancers just to be safe
        }

        if (dataRequestedL)
        {
            dataRequestedL = false;
            commsTimer = 0;

            // Voltage packets
            // the compiler produces more efficient code when loop indexes
            // here are uint16_t instead of uint8_t, for some reason
            for (uint16_t packet = 0; packet < 3u; packet++)
            {
                for (uint16_t n = 0; n < 4u; n++)
                {
                    txData[n * 2u] = voltage[(packet * 4u) + n] >> 8; // Top 8 bits
                    txData[(n * 2u) + 1u] = voltage[(packet * 4u) + n] & 0xFFu; // Bottom 8 bits
                }
                CanTX(moduleID + packet + 1u, 8);
                _delay_ms(1); // Brief intermission between packets?
            }

            // Temperature packet
            (void)memset(txData, 0, sizeof(txData)); // zero out unused
            txData[0] = LineariseTemp(temp[0]);
            txData[1] = LineariseTemp(temp[1]);
            CanTX(moduleID + BMS12_REPLY4, 8);

        }
        else if (dataRequestedH)
        {
            dataRequestedH = false;
            commsTimer = 0;

            // Voltage packets
            for (uint16_t packet = 0; packet < 3u; packet++)
            {
                for (uint16_t n = 0; n < 4u; n++)
                {
                    txData[n * 2u] = voltage[12u + (packet * 4u) + n] >> 8; // Top 8 bits
                    txData[(n * 2u) + 1u] = voltage[12u + (packet * 4u) + n] & 0xFFu; // Bottom 8 bits
                }
                CanTX(moduleID + 10u + packet + 1u, 8);
                _delay_ms(1); // Brief intermission between packets?
            }

            // Temperature packet
            (void)memset(txData, 0, sizeof(txData)); // zero out unused
            txData[0] = LineariseTemp(temp[2]);
            txData[1] = LineariseTemp(temp[3]);
            CanTX(moduleID + 10u + BMS12_REPLY4, 8);

        }
        else
        {
            _delay_ms(4); // Talking to LTC takes 27ms, so this makes it 31ms, which inverts to about 32Hz. Accuracy not important.
        }

        GetModuleID(); // Update in case it changed at runtime
    }
}

void CanTX(uint32_t packetID, uint8_t bytes)
{
    CANPAGE = 0x00; // Select MOB0 for transmission
    while ((CANEN2 & (1 << ENMOB0)) != 0)
    {} // Wait for MOB0 to be free
    CANSTMOB = 0x00;

    if (USE_29BIT_IDS != 0u) // CAN 2.0b is 29-bit IDs, CANIDT4 has bits 0-4 in top 5 bits, CANID3 has 5-12
    {
        CANIDT1 = packetID >> 21;
        CANIDT2 = packetID >> 13;
        CANIDT3 = packetID >> 5;
        CANIDT4 = (packetID & 0b00011111u) << 3u;
    }
    else // CAN 2.0a is 11-bit IDs, IDT1 has top 8 bits, IDT2 has bottom three bits BUT at top of byte!
    {
        CANIDT1 = (packetID >> 3u); // Packet ID
        CANIDT2 = (packetID & 0x07u) << 5;
        CANIDT3 = 0x00;
        CANIDT4 = 0x00;
    }

    for (uint8_t i = 0; i < bytes; i++)
    {
        CANMSG = txData[i];
    }

    // Enable transmission, 8-bit data
    CANCDMOB = (1u << CONMOB0) | (bytes << DLC0) | ((1u << IDE) * USE_29BIT_IDS);

    while (!(CANSTMOB & (1 << TXOK)) && !(CANSTMOB & (1 << AERR)))
    {} // Wait for transmission to finish (via setting of TXOK flag)

    CANCDMOB = 0x00; // Disable transmission
    CANSTMOB = 0x00; // Clear TXOK flag
}

// Function to convert ADC level to temperature. From datasheet for Epcos 100Kohm NTC B25/100 of 4540 K
// Temps:   -40  -30  -20  -10    0   10   20   30   40   50   60   70   80    90    100   110   120   130   140   150  160
// Rt/R25: 46.4 23.3 12.2  6.61 3.71 2.15 1.28 0.78 0.49 0.32 0.21 0.14 0.095 0.066 0.047 0.033 0.024 0.018 0.014 0.010 0.08
// ADC scale seems to be 0-2048, but slight drop due to input impedance makes scale 0-2030 or so

int LineariseTemp(uint16_t adc)
{
    static const uint16_t tempData[21] =
    { 1987, 1946, 1876, 1763, 1599, 1386, 1140, 890, 668, 492,
      352, 249, 176, 126, 91, 65, 48, 36, 28, 20, 16
    };
    int ret = 0;

    // check for temp sanity before converting
    // original comment said: Temporary, sometimes unplugged temp gives like -30degC instead of 0
    if (!((adc > 1950u) || DISABLE_TEMPS))
    {
        for (uint8_t n = 0; n < 20u; n++)
        {
            if ((adc <= tempData[n]) && (adc > tempData[n + 1u])) // We're between samples
            {
                // Calculate linear interpolation
                ret = (n * 10u) + ((10u * (adc - tempData[n])) / (tempData[n + 1u] - tempData[n]));
            }
        }
    }

    return ret;
    // old comment: i.e "not available" if the algorithm below doesn't find it (-40 to 160degC)
}

void SetupPorts(void)
{
    DDRB = 0b11001000; // PB3 = SCKI, PB6 = CSBI2, PB7 = SDI2
    DDRC = 0b01100010; // PC1 = GREEN, PC5 = CSBI, PC6 = SDI
    DDRD = 0b00001001; // PD0 = VIA2, PD3 = RED

    // Pull-ups for module ID selectors
    PORTB = 0b00000100;
    PORTC = 0b00000000;
    PORTD = 0b11100000;

    // CAN init stuff. Further info on page 203 of ATmega16M1 manual
    CANGCON = (1<<SWRES); // Software reset
    CANTCON = 0; // CAN timing prescaler set to 0

    if (CAN_BAUD_RATE == 1000)
    {
        CANBT1 = 0x00;
    }
    else if (CAN_BAUD_RATE == 500)
    {
        CANBT1 = 0x02;
    }
    else if (CAN_BAUD_RATE == 250)
    {
        CANBT1 = 0x06;
    }
    else
    {
        CANBT1 = 0x0E;
    }
    CANBT2 = 0x04;

    if (CAN_BAUD_RATE == 1000)
    {
        CANBT3 = 0x12;
    }
    else
    {
        CANBT3 = 0x13;
    }

    for (uint8_t mob = 0; mob < 6u; mob++)
    {
        CANPAGE = (mob << 4); // Select MOB 0-5
        CANCDMOB = 0x00; // Disable MOB
        CANSTMOB = 0x00; // Clear MOB status register
    }

    CANPAGE = (1 << MOBNB0); // Select MOB1
    CANIE2 = (1 << IEMOB1); // Enable interrupts on MOB1 for reception and transmission
    CANGIE = (1 << ENIT) | (1 << ENRX); // Enable interrupts on receive and transmit
    CANIDM1 = CANIDM2 = CANIDM3 = CANIDM4 = 0x00; // CAN ID mask, zero will let all IDs pass
    // Enable reception, 11-bit IDE, 8-bit data length
    CANCDMOB = (1u << CONMOB1) | (8u << DLC0) | ((1u << IDE) * USE_29BIT_IDS);
    CANGCON |= (1 << ENASTB); // Enable mode. CAN channel enters enable mode after 11 recessive bits have been read
}

void GetModuleID(void)
{
    uint16_t rotarySwitch = 0;

    if (!MOD_ID_NUM1) { rotarySwitch += 1u; }
    if (!MOD_ID_NUM2) { rotarySwitch += 2u; }
    if (!MOD_ID_NUM4) { rotarySwitch += 4u; }
    if (!MOD_ID_NUM8) { rotarySwitch += 8u; }
    // Original comment on the below said:
    // "Atomic assignment in case of interrupt"
    // But that code is not an atomic assignment
    // If it really needs to be protected from interrupt
    // it needs a critical section
    // TODO: determine if a critical section is needed
    moduleID = BASE_ID + (rotarySwitch * 10u);
}

void WriteSPIByte(uint8_t byte)
{
    uint8_t mask = 0b10000000u; // MSB first
    for (uint8_t n = 0; n < 8u; n++) // 8 bits = 1 byte
    {
        // Prepare pin
        if ((byte & mask) > 0u) // Bit is on
        {
            SDI_PORT |= SDI;
        }
        else
        {
            SDI_PORT &= ~SDI;
        }

        _delay_us(2);
        SCKI_PORT |= SCKI; // Clock goes high = register SDI state
        _delay_us(2);
        SCKI_PORT &= ~SCKI; // Clock goes low
        mask = mask>>1;
    }
}

uint8_t ReadSPIByte(void)
{
    uint8_t byte = 0;
    uint8_t mask = 0b10000000u;
    for (uint8_t n = 0; n < 8u; n++)
    {
        _delay_us(2);
        SCKI_PORT |= SCKI; // Clock goes high = register SDI state
        _delay_us(2);
        if ((SDO_PORT & SDO) != 0)
        {
            byte += mask;
        }
        SCKI_PORT &= ~SCKI; // Clock goes low
        mask = mask >> 1;
    }
    return byte;
}

void WriteSPIByte2(uint8_t byte)
{
    uint8_t mask = 0b10000000u; // MSB first
    for (uint8_t n = 0; n < 8u; n++) // 8 bits = 1 byte
    {
        // Prepare pin
        if ((byte & mask) > 0u) // Bit is on
        {
            SDI2_PORT |= SDI2;
        }
        else
        {
            SDI2_PORT &= ~SDI2;
        }

        _delay_us(2);
        SCKI2_PORT |= SCKI2; // Clock goes high = register SDI state
        _delay_us(2);
        SCKI2_PORT &= ~SCKI2; // Clock goes low
        mask = mask>>1;
    }
}

uint8_t ReadSPIByte2(void)
{
    uint8_t byte = 0;
    uint8_t mask = 0b10000000u;
    for (uint8_t n = 0; n < 8u; n++)
    {
        _delay_us(2);
        SCKI2_PORT |= SCKI2; // Clock goes high = register SDI state
        _delay_us(2);
        if ((SDO2_PORT & SDO2) != 0)
        {
            byte += mask;
        }
        SCKI2_PORT &= ~SCKI2; // Clock goes low
        mask = mask >> 1;
    }
    return byte;
}

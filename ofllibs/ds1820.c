#include <mc1322x.h>
#include <board.h>
#include "config.h"

#include <rtc.h>
#include <stdio.h>

#include "ds1820.h"
#include "utils.h"
#include "crc8.h"

// Set to 1 if you use DS18S20 family (untested)
#define DS18S20_FAMILY_CODE 0


// https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf

// ds1820_start : Power on the 2 GPIOs for driving device
void ds1820_start(void) {
    DBG("ds1820: ds1820_start\r\n")

	// Init two GPIOs (Power + 1wire Data)
    setPinGpio(TEMP_POWER_PIN, GPIO_DIR_OUTPUT);
    setPinGpio(TEMP_PIN, GPIO_DIR_OUTPUT);
    digitalWrite(TEMP_POWER_PIN, 1);
    digitalWrite(TEMP_PIN, 1);
}

// ds1820_stop : Power off the device
void ds1820_stop(void){
    DBG("ds1820: ds1820_stop\r\n")
	// Pull down to save energy ?
    digitalWrite(TEMP_POWER_PIN, 0);
    digitalWrite(TEMP_PIN, 0);
}


int8_t ds1820_readTemp(uint8_t* subzero, uint8_t* cel, uint8_t* cel_frac_bits){
    DBG("ds1820_readTemp\r\n")

    uint8_t sp[9];              // scratchbox
    int8_t ret =0;

	uint8_t cc; 
	// Request measurement
    OneWireReset(TEMP_PIN);
    OneWireOutByte(TEMP_PIN, 0xcc); // skip rom: address all devices simultaneously-start all temp. conversion
    OneWireOutByte(TEMP_PIN, 0x44); // perform temperature conversion
	// Pause 750 ms
    int z;for (z=0; z<1000; z++) {delayMicroseconds(1000);}         // 1 second may be enough for concertion!  
	// Read 
    OneWireReset(TEMP_PIN);
    OneWireOutByte(TEMP_PIN, 0xcc); // skip rom
    OneWireOutByte(TEMP_PIN, 0xbe); // read scratchpad command
    for (cc=0; cc < 9; cc++) sp[cc] = OneWireInByte(TEMP_PIN);
    uint8_t computeCrc8 = crc8 (sp, 8);

	DBG("ds1820: sequence ");
    int k; for (k=0;k<9;k++){DBG("%02X ",sp[k]);}
	DBG("\r\n");

    DBG("ds1820: readTemp: crc8_read:%02X crc8_computed:%02X\r\n", computeCrc8, sp[8])
    if (computeCrc8 == sp[8]){

        uint16_t meas = sp[0];  // LSB
        meas |= ( (uint16_t)sp[1] ) << 8; // MSB

        //  only work on 12bit-base
        if ( DS18S20_FAMILY_CODE ) { // 9 -> 12 bit if 18S20
            /* Extended res. measurements for DS18S20 contributed by Carsten Foss */
            meas &= (uint16_t) 0xfffe;    // Discard LSB, needed for later extended precicion calc
            meas <<= 3;                   // Convert to 12-bit, now degrees are in 1/16 degrees units
            meas += ( 16 - sp[6] ) - 4;   // Add the compensation and remember to subtract 0.25 degree (4/16)
        }

        // check for negative
        if ( meas & 0x8000 )  {
            *subzero=1;      // mark negative
            meas ^= 0xffff;  // convert to positive => (twos complement)++
            meas++;
        }
        else {
            *subzero=0;
        }

        *cel  = (uint8_t)(meas >> 4);
        *cel_frac_bits = (uint8_t)(meas & 0x000F) * 100  /16;

        DBG("ds1820: readTemp: result: cel:%d cel_frac_bits:%d\r\n", *cel, *cel_frac_bits)

        ret=1;
    }

    return(ret);
}

void OneWireReset(uint8_t Pin) // reset.  Should improve to act as a presence pulse
{
    setPinGpio(Pin, GPIO_DIR_OUTPUT); // bring low for 500 us
    digitalWrite(Pin, 0);
    delayMicroseconds(500);
    digitalWrite(Pin, 1);
    delayMicroseconds(500); // Wait for the ds1820 to sent its pulse
}

void OneWireOutByte(uint8_t Pin, uint8_t d) // output byte d (least sig bit first).
{
    uint8_t n;

    setPinGpio(Pin, GPIO_DIR_OUTPUT);
    for(n=8; n!=0; n--)
    {
        if ((d & 0x01) == 1)  // test least sig bit
        {
            digitalWrite(Pin, 0);
            delayMicroseconds(7);
            digitalWrite(Pin, 1);
            delayMicroseconds(64);
        }
        else
        {
            digitalWrite(Pin, 0);
            delayMicroseconds(60);
            digitalWrite(Pin, 1);
            delayMicroseconds(10);
        }

        d=d>>1; // now the next bit is in the least sig bit position.
    }

}

uint8_t OneWireInByte(uint8_t Pin) // read byte, least sig byte first
{
    uint8_t d, n, b;

    d=0;
    for (n=0; n<8; n++)
    {
        digitalWrite(Pin, 0);
        setPinGpio(Pin, GPIO_DIR_OUTPUT);
        delayMicroseconds(5);
        setPinGpio(Pin, GPIO_DIR_INPUT);
        delayMicroseconds(5);
        b = digitalRead(Pin);
        delayMicroseconds(50);
        d = (d >> 1) | (b<<7); // shift d to right and insert b in most sig bit position
    }
    return(d);
}

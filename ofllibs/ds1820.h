#ifndef DS1820_H
# define DS1820_H

#define TEMP_POWER_PIN  28
#define TEMP_PIN  29

// KBI6 GPIO28
// KBI7 GPIO29


/* Prototypes */
void OneWireReset(uint8_t Pin);
void OneWireOutByte(uint8_t Pin, uint8_t d);
uint8_t OneWireInByte(uint8_t Pin);
void ds1820_start(void);
void ds1820_stop(void);
int8_t ds1820_readTemp(uint8_t* subzero, uint8_t* cel, uint8_t* cel_frac_bits);

#endif

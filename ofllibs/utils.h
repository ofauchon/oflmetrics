#ifndef UTIL_H
#define UTIL_H


#include <board.h>

#define GPIO_DIR_INPUT 0
#define GPIO_DIR_OUTPUT 1
#define ORDER_LSBFIRST 1
#define ORDER_MSBFIRST 2


#define CAPA_POWER 1
#define CAPA_LIGHT 2
#define CAPA_TEMP 4



/*
Configuration ib NVR
NVR @ offset 0x1f000

 sig         smac               txpower radiochan capabilities  reserved       securekey (8)
|   |  |---------------------|   |         |           |       |             |--------------|
XX XX  XX XX XX XX XX XX XX XX   XX        XX          XX      XX            XX ... 32 ... XXXX
*/
 

/* PACKET STUFF */
typedef struct {
	uint8_t		signature[2];   	// F00F when eeprom contains data
        uint8_t         smac[8];		// ID (MAC) of node 
        uint8_t         span[2];		// ID (MAC) of node 
        uint8_t         txpower;		// txpower
        uint8_t         radiochan;		// Current radio channel 
        uint8_t         capa[2];		// Capabilities
        uint8_t         securekey[32];		// Shared key

        uint8_t 	low_uptime_flag;    	// Flag used to detect fast OFF-ON-OFF of the board
        uint8_t 	low_uptime_counter; 	// After a couple of ON/OFF/ON, we may reset to factory  
} config_t;



/* Prototypes */
void digitalWrite(uint8_t pin, uint8_t val);
void setPinGpio(uint8_t pin, uint8_t dir);
uint8_t digitalRead(uint8_t pin);
void delay(int d);
void delayMicroseconds(uint16_t d);
void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);
uint16_t hexToInt(char* c, uint8_t len);
int8_t read_config(config_t *t);
int8_t write_config(config_t *t);
void dump_config(config_t myconfig);
void default_config(config_t *t);
char* my_itoa(int16_t i, char b[]); 


#endif

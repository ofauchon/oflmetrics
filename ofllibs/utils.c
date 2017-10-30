/*
Copyright (C) 2013, 2014 Olivier Fauchon
This file is part of OFLMetrics.

OFLMetrics is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OFLMetrics is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
*/

#include <mc1322x.h>
#include <board.h>

#include <gpio.h>
#include <gpio-util.h>
#include "config.h"
#include "utils.h"

#include <string.h>
#include <stdio.h>


/*
 * Read configuration from NVM
 */
int8_t read_config(config_t *t)
{
        nvmType_t type;
        nvmErr_t err; 

        uint8_t buf[NVM_CONFIG_SZ];
        
        err = nvm_detect(gNvmInternalInterface_c, &type);
        if (err) return -1;  
        nvm_setsvar(0);

        err = nvm_read(gNvmInternalInterface_c, type, buf, NVM_CONFIG_POS, NVM_CONFIG_SZ);
        if (err) return -2; 

        if (buf[0]!=0xF0 && buf[1]!= 0x0F) {
            DBG("! no_conf_sig_nvm\r\n")
            return -3; // NO CONFIG SIGNATURE
        }

        t->signature[0]=buf[0];t->signature[1]=buf[1];        
		memcpy((uint8_t*)t->smac, buf+2,8);
        t->radiochan=buf[10];
        t->txpower=buf[11];
        t->capa[0]=buf[12];
        t->capa[1]=buf[13];
        // Secure Key 
        memcpy( (uint8_t*)(buf+14),t->securekey,8);
        t->low_uptime_flag = buf[22];
        t->low_uptime_counter = buf[23];
        return 1; 
}

/*
 * Write configuration from NVM
 */
int8_t write_config(config_t *t)
{
        nvmType_t type;
        nvmErr_t err; 
        uint8_t buf[NVM_CONFIG_SZ];

        buf[0]=t->signature[0]; buf[1]=t->signature[0];  // SET CONFIG SIGNATURE
		memcpy((uint8_t*)buf+2, t->smac,8);
        buf[10]=t->radiochan;
        buf[11]=t->txpower;
        buf[12]=t->capa[0]; 
        buf[13]=t->capa[1]; 
        memcpy( (uint8_t*)(buf+14),t->securekey,8);
        buf[22]=t->low_uptime_flag;
        buf[23]=t->low_uptime_counter;


        err = nvm_detect(gNvmInternalInterface_c, &type);
        if (err) return -1;  

        err = nvm_erase(gNvmInternalInterface_c, type, 1 << NVM_CONFIG_POS/4096);
        if (err) return -2;  

        err = nvm_write(gNvmInternalInterface_c, type, buf, NVM_CONFIG_POS, NVM_CONFIG_SZ);
        if (err) return -3;  

        return 1; 
}

void default_config(config_t *myconfig)
{
    // Signature ()
            myconfig->signature[0]=0xF0; myconfig->signature[1]=0x0F; 	// 0xF00F when present
    // Mac address
			memset(myconfig->smac,8,0);
            myconfig->smac[7]=DEFAULT_NODENO;
	// Power 0x12
            myconfig->txpower=0x12;
	// Channel 0 
            myconfig->radiochan=0x00;
	// Capabilities 0xFFFF
            myconfig->capa[0]=CAPA_TEMP; 
            myconfig->capa[1]=0x00;

	// When 1: this means the board was tuned OFF ON OFF  in a short interval
            myconfig->low_uptime_counter=0; 
}



/*
 *  TESTS NVM CONFIG
 */
void dump_config(config_t myconfig)
{
    DBG("- signature: 0x%02X%02X\r\n", myconfig.signature[0] ,myconfig.signature[1] );
    DBG("- txpower: 0x%02X, radiochan: 0x%02X\r\n", myconfig.txpower ,myconfig.radiochan );
    DBG("- low_uptime_flag: 0x%02X, low_uptime_counter: 0x%02X\r\n", myconfig.low_uptime_flag ,myconfig.low_uptime_counter );
    DBG("- span:0x%02X%02X\r\n", myconfig.span[0], myconfig.span[1]);
    DBG("- smac:0x%02X%02X%02X%02X%02X%02X%02X%02X\r\n capa:0x%02X%02X", myconfig.smac[0], myconfig.smac[1],myconfig.smac[2],myconfig.smac[3], myconfig.smac[4], myconfig.smac[5],myconfig.smac[6],myconfig.smac[7],myconfig.capa[0], myconfig.capa[1]);
    DBG("- securekey:0x");
    int i; 
    for (i=0;i<7;i++){
        DBG("%02X ",myconfig.securekey[i]);
    }
    DBG("\r\n");
}



/* Code */
void setPinGpio(uint8_t pin, uint8_t dir)
{ 
  gpio_select_function(pin, 3);  // Function 3 : GPIO

  if (dir == GPIO_DIR_OUTPUT){
     gpio_set_pad_dir(pin, PAD_DIR_OUTPUT);
  } else {
     gpio_set_pad_dir(pin, PAD_DIR_INPUT); 
  }
}



void digitalWrite(uint8_t pin, uint8_t val)
{ 
  if (val == 0) gpio_reset(pin);
  else if (val == 1) gpio_set(pin);
}

uint8_t digitalRead(uint8_t pin)
{ 
  return gpio_read(pin);
}



void delay(int d){
	int j;
	for (j=0; j<d; j++) {
	 continue;
	}
}

#define CYCLES_PER_US 3
#define CORRECTION 5
void delayMicroseconds(uint16_t d){
	uint16_t i;
        for (i=0; i< ((d - CORRECTION) * CYCLES_PER_US) ; i++) {
		asm("mov r0,r0"); 
	}

}


void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val)
{
        uint8_t i;

        for (i = 0; i < 8; i++)  {
                if (bitOrder == ORDER_LSBFIRST)
                        digitalWrite(dataPin, !!(val & (1 << i)));
                else    
                        digitalWrite(dataPin, !!(val & (1 << (7 - i))));
    
                digitalWrite(clockPin, 1); 
                digitalWrite(clockPin, 0);    
        }   
}


uint16_t hexToInt(char* c, uint8_t len){
        int16_t res = 0;  
        int pos;
        char curchar;
        int curval;
        int order=0; 

        for (pos= len-1; pos>=0; pos--){
                curchar = c[pos];

                if (curchar>='0' && curchar<='9') {
                        curval =  curchar - '0';
                } else if (curchar>='A' && curchar<='F') {
                        curval = curchar - 'A' + 10 ;
                } else {
                        return(-1); 
                }   
                res += curval * ( 1 <<  ( 4* order) );  
                order ++; 
    
        }   
        return(res);
}



char* my_itoa(int16_t i, char b[]){
	char const digit[] = "0123456789";
    char* p = b;
    if(i<0){
        *p++ = '-';
        i *= -1; 
    }   
    int shifter = i;
    do{ //Move to where representation ends
        ++p;
        shifter = shifter/10;
    }while(shifter);
    *p = '\0';
    do{ //Move back, inserting digits as u go
        *--p = digit[i%10];
        i = i/10;
    }while(i);
    return b;
}


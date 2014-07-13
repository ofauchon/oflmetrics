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
            DBG("NO 0xF00F signature in NVM ")
            return -3; // NO CONFIG SIGNATURE
        }

        t->signature[0]=buf[0];t->signature[1]=buf[1];        
        t->smac[3]=buf[2];t->smac[2]=buf[3];
        t->smac[1]=buf[4];t->smac[0]=buf[5];
        t->radiochan=buf[6];
        t->txpower=buf[7];
        t->capa[0]=buf[8];
        t->capa[1]=buf[9];
        // Secure Key 
        memcpy( (uint8_t*)(buf+10),t->securekey,8);
        t->low_uptime_flag = buf[18];
        t->low_uptime_counter = buf[19];
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

        buf[2]=t->smac[3]; buf[3]=t->smac[1];
        buf[4]=t->smac[1]; buf[5]=t->smac[0]; 

        buf[6]=t->radiochan;
        buf[7]=t->txpower;
        buf[8]=t->capa[0]; 
        buf[9]=t->capa[1]; 

        buf[18]=t->low_uptime_flag;
        buf[19]=t->low_uptime_counter;

        memcpy( (uint8_t*)(buf+10),t->securekey,8);

        err = nvm_detect(gNvmInternalInterface_c, &type);
        if (err) return -1;  

        err = nvm_erase(gNvmInternalInterface_c, type, 1 << NVM_CONFIG_POS/4096);
        if (err) return -2;  

        err = nvm_write(gNvmInternalInterface_c, type, buf, NVM_CONFIG_POS, NVM_CONFIG_SZ);
        if (err) return -3;  

        return 1; 
}


/*
 *  TESTS NVM CONFIG
 */
void dump_config(config_t myconfig)
{
    printf("- signature: 0x%02X%02X\r\n", myconfig.signature[0] ,myconfig.signature[1] );
    printf("- txpower: 0x%02X, radiochan: 0x%02X\r\n", myconfig.txpower ,myconfig.radiochan );
    printf("- low_uptime_flag: 0x%02X, low_uptime_counter: 0x%02X\r\n", myconfig.low_uptime_flag ,myconfig.low_uptime_counter );
    printf("- smac:0x%02X%02X%02X%02X\r\n capa:0x%02X%02X", myconfig.smac[0], myconfig.smac[1],myconfig.smac[2],myconfig.smac[3], myconfig.capa[0], myconfig.capa[1]);
    printf("- securekey:0x");
    int i; 
    for (i=0;i<7;i++){
        printf("%02X ",myconfig.securekey[i]);
    }
    printf("\n");
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


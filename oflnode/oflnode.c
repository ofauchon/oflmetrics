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

#include <string.h>
#include <stdio.h>

#include "libs/config.h"
#include "libs/lcd.h"
#include "libs/ds1820.h"
#include "libs/utils.h"
#include "libs/utils-paquet.h"

#include <adc.h>

#define SERIAL_SPEED 115200

config_t myconfig; 

// Global variables
volatile uint32_t cntr, cntr_msg_sent;
volatile uint8_t clock_d,clock_h,clock_m,clock_s, clock_100ms;

// RX/TX paquet 
paquet tx_paquet; 
paquet rx_paquet;

#define RX_LED_PIN 44
#define TX_LED_PIN 44

volatile uint8_t state_init;


/*
 * Free RX packets & return one.
 */
void process_rx_packet(volatile packet_t *prx)
{
    DBG("Packet received.\r\n");
    if (prx->length >0 ){  
            paquet pqrx;
            printf("RX:%s\r\n",pqrx.data);
            packet2paquet(prx, &pqrx);

            // FIXME : hardcoded size of 100 below
            char data[100];
            memset(data,0,sizeof(data));
            int n;
            paquet pqtx; 
            if ( (n = strncmp((char*)pqrx.data,"PING",pqrx.datalen))  ){
                sprintf((char*)pqtx.data,"PONG");
            }
            memcpy(pqtx.smac, myconfig.smac,4); // set smac based on config source mac
	    memcpy(pqtx.dmac, pqrx.smac,4);     // set dmac based on rx packet smac
            memcpy(pqtx.data,data, strlen(data)); //  copy payload to paquet
            pqtx.datalen=strlen(data); // Set payload size

	    // convert paquet to libmc1322x struct packet, and send it
            volatile packet_t *ptx;
            ptx=get_free_packet();
            paquet2packet(&pqtx,ptx);
            tx_packet(ptx);

    }
    free_packet(prx);
}



/*
 * Blink Led on RX
 */
void maca_rx_callback(volatile packet_t *p) {
    (void)p;
    gpio_data_set(1ULL<< RX_LED_PIN);
    gpio_data_reset(1ULL<< RX_LED_PIN);
}


/*
 * Timer callback
 */
void tmr0_isr(void)
{
    cntr++;
    clock_100ms+=1;

    //process_rx_packets();
    if (clock_100ms == 10) {
        clock_s++;
        clock_100ms=0;
    }


    if (clock_s == 60) { clock_m++; clock_s=0;}
    if (clock_m == 60) { clock_h++; clock_m=0;}
    if (clock_h == 24) { clock_d++; clock_h=0;}

    // Restart counter
    *TMR0_SCTRL = 0;
    *TMR0_CSCTRL = 0x0040; /* clear compare flag */

}

void timers_init(void)
{
    /* timer setup */
    /* CTRL */
#define TT_COUNT_MODE 1      /* use rising edge of primary source */
#define TT_PRIME_SRC  0xf    /* Perip. clock with 128 prescale (for 24Mhz = 187500Hz)*/
#define TT_SEC_SRC    0      /* don't need this */
#define TT_ONCE       0      /* keep counting */
#define TT_LEN        1      /* count until compare then reload with value in LOAD */
#define TT_DIR        0      /* count up */
#define TT_CO_INIT    0      /* other counters cannot force a re-initialization of this counter */
#define TT_OUT_MODE   0      /* OFLAG is asserted while counter is active */

    *TMR_ENBL     = 0;                    /* tmrs reset to enabled */
    *TMR0_SCTRL   = 0;
    *TMR0_CSCTRL  = 0x0040;
    *TMR0_LOAD    = 0;                    /* reload to zero */
    *TMR0_COMP_UP = 18750;                /* trigger a reload at the end */
    *TMR0_CMPLD1  = 18750;                /* compare 1 triggered reload level, 10HZ maybe? */
    *TMR0_CNTR    = 0;                    /* reset count register */
    *TMR0_CTRL    = (TT_COUNT_MODE<<13) | (TT_PRIME_SRC<<9) | (TT_SEC_SRC<<7) | (TT_ONCE<<6) | (TT_LEN<<5) | (TT_DIR<<4) | (TT_CO_INIT<<3) | (TT_OUT_MODE);
    *TMR_ENBL     = 0xf;                  /* enable all the timers --- why not? */
}


/*
 *  100ms sleep 
 */
void wait100ms(uint32_t delta){
    uint32_t oldc = cntr;
    uint32_t del;
    //DBG("start wait\r\n");

    do {
        del = cntr - oldc;
    } while (del < delta  );
    //DBG("D: %d %d \r\n", (int)cntr, (int)oldc);

}


uint8_t get_lumi(uint16_t *plumi){
#define LUMICALC_SAMPLES 5
    uint32_t tt =0;
    uint8_t tc =0;
    for (tc=0; tc<LUMICALC_SAMPLES; tc++){
        adc_service();
        tt += adc_reading[0];
        wait100ms(2);
    }
    *plumi = (uint16_t)(tt/LUMICALC_SAMPLES) ;
    return (1);
}


/*
 * Power consumption is determined by the amplitude
 * of the sine.
 * Read as much as samples as period length, to get min, max & determine amplitude
 */
uint8_t get_power(uint16_t *ppower){
#define POWER_SAMPLES 5
#define POWER_ADC_CHAN 1
    uint32_t tmin=0, tmax =0;
    uint8_t tc =0;
    for (tc=0; tc<POWER_SAMPLES; tc++){
        adc_service();
        if (tc == 0 ) {
            tmin = adc_reading[POWER_ADC_CHAN];
            tmax = adc_reading[POWER_ADC_CHAN];

        } else {
            if (adc_reading[POWER_ADC_CHAN] < tmin ) tmin = adc_reading[POWER_ADC_CHAN];
            if (adc_reading[POWER_ADC_CHAN] > tmax ) tmax = adc_reading[POWER_ADC_CHAN];
        }
    }
    *ppower = (uint16_t)(tmax - tmin ) ;
    return (1);
}



uint16_t batteryLevel(void){

    uint8_t i,j;
    uint32_t adc[9];
    adc_init();
    for(j=0;j<9;j++) adc[j] = 0;

    // Read 8 times to
    for(i=0;i<8;i++) {
        adc_reading[8]=0;
        while (adc_reading[8]==0) adc_service(); // Convertion done if != 0
        for(j=0;j<9;j++) adc[j] += adc_reading[j]; // Copy reading to array
    }
    adc_disable();
    for (j=0;j<8;j++) adc_reading[j]=(1200*adc[j]/adc[8]);
    /* Correct Vcc for what appears to be a 120 millivolt excess */
    adc_reading[8]=1200*0xfff*8/adc[8]-120;

    return adc_reading[8];
}

void hibernate(uint8_t p_sec)
{

    // Hibernate (See sleep example of libmc1322x for details
    *CRM_WU_CNTL = 0x1;
    *CRM_WU_TIMEOUT =  p_sec * 2000; /* 60s */
    *CRM_SLEEP_CNTL = 0x71;
    while((*CRM_STATUS & 0x1) == 0) { continue; }
    *CRM_STATUS = 1;
    // Sleep
    while((*CRM_STATUS & 0x1) == 0) { continue; }
    *CRM_STATUS = 1;

}

void reset_watchdog(void)
{
    cop_timeout_ms(1);
    CRM->COP_CNTLbits.COP_EN = 1;
    while (1) continue;
}


/*
 * Main
 */
void main(void) {

    volatile packet_t *txp;
    volatile packet_t *rxp;

    // Mac address
    tx_paquet.dmac[0] = 0xFF; tx_paquet.dmac[1] = 0xFF;   tx_paquet.dmac[2] = 0xFF;  tx_paquet.dmac[3] = 0xFF;

    cntr=0;
    cntr_msg_sent=0;


    // Wall clock
    clock_h=0;
    clock_m=0;
    clock_s=0;

    // Prepare timers.
    timers_init();
    enable_irq(TMR);

    // UART
    if (DODEBUG) uart_init(UART1, SERIAL_SPEED);
    DBG("OFLnode start\r\n");

    // uController init
    trim_xtal();
    vreg_init();

    state_init=0; 

    // Read config from NVR if possible
    int res;
    res=read_config(&myconfig);
    if ( res < 0) { // Problem reading configuration
        DBG("read_config NOT OK : err:%d , using defaults\r\n", res);
        if ( res == -3){ // No configuration signature in NVR
	    default_config(&myconfig);
        }
    }

    // Ugly part of the boot process : Detecting reset to factory
    if (myconfig.low_uptime_flag==0x01){  // NVM tells us the previous runtime was short ... 
        myconfig.low_uptime_counter++;   
        if (myconfig.low_uptime_counter> RESET_REBOOT_COUNT) state_init=1;  // Too much ON/OFF/ON/OFF short cycles, we enter init state
    } else  {
        myconfig.low_uptime_counter=0;   // The device 
    }


    if(state_init==0){ // Update counters only if not already in init mode
	int z; 
	for (z=0;z<20;z++){ // Blink 20x to notify for NVR config reset
            digitalWrite(RX_LED_PIN, 1); wait100ms(2);
            digitalWrite(RX_LED_PIN, 0); wait100ms(2);
	}
        myconfig.low_uptime_flag=0x1;
        res=write_config(&myconfig);
    }else { 
        DBG ("Reset configuration and write to NVR");
    }

    dump_config(myconfig);






    // Init Radio
    DBG("Init radio\r\n");
    maca_init();
    set_channel(myconfig.radiochan);
    set_power(myconfig.txpower);

    //RX TX LED
    setPinGpio(RX_LED_PIN, GPIO_DIR_OUTPUT);
    setPinGpio(TX_LED_PIN, GPIO_DIR_OUTPUT);

    // Init sensors depending on configuration
    if ( ( myconfig.capa[0] | CAPA_TEMP  )   ) ds1820_start();
    if ( ( myconfig.capa[0] | CAPA_LIGHT  )   )   adc_init();
    if ( ( myconfig.capa[0] | CAPA_POWER )  ) adc_init();

    maca_on();

    // Main loop
    while(1){
        DBG("Main loop cycle #%u\r\n",(unsigned int) cntr);

        memset(tx_paquet.data,0,PAQUET_MAX_DATASIZE); // Message buffer

        // Add battery information every 10 cycles
        if ( (cntr_msg_sent % 10 ) == 0 ){
            uint16_t t_batl= batteryLevel();
            sprintf((char*)tx_paquet.data + strlen((char*)tx_paquet.data), "FWVER:%04d;BATLEV:%u;", FW_VER, t_batl  );
        }

        // Add temperature information if node has capability
        if ( myconfig.capa[0] | CAPA_TEMP ) {
            uint8_t  m_cel, m_cel_frac, m_cel_sign;
            if (ds1820_readTemp(&m_cel_sign, &m_cel, &m_cel_frac) == 1){
                sprintf((char*)tx_paquet.data + strlen((char*)tx_paquet.data), "TEMP:%c%d.%02d;",
                        m_cel_sign == 1 ? '-' : '+',
                        m_cel, m_cel_frac);
            }
        }

        // Add light level information if node has capability
        if ( myconfig.capa[0] | CAPA_LIGHT ) {
            uint16_t m_lumi;
            if (get_lumi(&m_lumi) == 1){
                sprintf((char*)tx_paquet.data + strlen((char*)tx_paquet.data), "LUMI:+%05u;",m_lumi);
            }
        }

        // Add power information if node has capability
        if ( myconfig.capa[0] | CAPA_POWER ) {
            uint16_t m_power;
            if ( get_power(&m_power) == 1 ){
                sprintf((char*)tx_paquet.data + strlen((char*)tx_paquet.data), "POWER:+%05u;",m_power);

            }
        }
 
        // Compute data length
        tx_paquet.datalen = strlen((char*)tx_paquet.data);


        check_maca();
        // Time to RX
        while (( rxp = rx_packet()))
        {
            DBG("Packet received\r\n");

            // DO SOMETHING
            // Blink LED x2
            digitalWrite(RX_LED_PIN, 1); wait100ms(2);
            digitalWrite(RX_LED_PIN, 0); wait100ms(2);
            digitalWrite(RX_LED_PIN, 1); wait100ms(2);
            digitalWrite(RX_LED_PIN, 0);

            process_rx_packet(rxp);
            free_packet(rxp);
        }

        // Send packet data
        txp = get_free_packet();
        if (txp == NULL) {
            DBG("No radio free_packets...\r\n");
            goto noprocess;
        } else {
          //  DBG("Send packet\r\n");
            memcpy (tx_paquet.smac, myconfig.smac, 4);
            if (paquet2packet( &tx_paquet,txp)){
                tx_packet(txp);
                cntr_msg_sent++;
            }
        }

	// If 200 main cycles were executed, clear low uptime flag in NVR
        if (myconfig.low_uptime_flag==0x01 &&  cntr > 200 ){
            DBG("Clean low_uptime_counter flag\r\n");
            myconfig.low_uptime_flag=0x00; 
            write_config(&myconfig);
        }

        // Reset every 10h to avoid freeze bug
        if (cntr_msg_sent>600) reset_watchdog();

noprocess:
        // Blink LED
        digitalWrite(RX_LED_PIN, 1);
        wait100ms(2);
        digitalWrite(RX_LED_PIN, 0);

// We can either use powersave functions, or sleep.
#ifdef HIBERNATE
        hibernate(HIBERNATE_DELAY);wait100ms(2);
#else
        wait100ms(HIBERNATE_DELAY * 10);
#endif

    }

}


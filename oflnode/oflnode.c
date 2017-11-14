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

/*
 * 104 => Enhance ds18x20 measure
 */

#define FW_VER 104

#include "libs/config.h"
#include "libs/ds1820.h"
#include "libs/i2c_bme280.h"
#include "libs/utils.h"
#include "libs/utils-paquet.h"

#include <adc.h>

#define SERIAL_SPEED 115200
#define DEEP_SLEEP 1  // 1 For hardware sleep, 0 for simple cpu loop sleep
#define SLEEP_DELAY 60

// Node ID 
#ifndef NODE_ID
#warning '** Using default NODE_NO=0x99'
#define NODE_ID 0x99
#endif



config_t myconfig; 

// Global variables
volatile uint32_t tmr_cntr, cntr_msg_sent, awake_sec, main_loop_count;
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
    DBG("@process_rx_packet\r\n")
    if (prx->length >0 ){  
            paquet pqrx;
            printf("  RX Payload: '%s'\r\n",pqrx.data);
            packet2paquet(prx, &pqrx);

            // FIXME : hardcoded size of 100 below
            char data[100];
            memset(data,0,sizeof(data));
            int n;
            paquet pqtx; 
            if ( (n = strncmp((char*)pqrx.data,"PING",pqrx.datalen))  ){
                sprintf((char*)pqtx.data,"PONG");
            }
            memcpy(pqtx.smac, myconfig.smac,8); // set smac based on config source mac
		    memcpy(pqtx.dmac, pqrx.smac,8);     // set dmac based on rx packet smac
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
    tmr_cntr++;
    clock_100ms+=1;

    //process_rx_packets();
    if (clock_100ms == 10) {
        awake_sec++;
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

/*
 * Timers initialisation
 */
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
 *  Helper function for 100ms cpu sleep 
 *  FIXME: Not sure this works when tmr_cntr is about to overflow
 */
void wait100ms(uint32_t delta){
    uint32_t oldc = tmr_cntr;
    uint32_t del;
    do {
        del = tmr_cntr - oldc;
    } while (del < delta  );
}

/*
 * Get luminosity / ambian light) level
 * Use adc to sample light sensor LUMICALC_SAMPLES times
 * Then returns average value
 */ 
#define LUMICALC_SAMPLES 5
uint8_t get_lumi(uint16_t *plumi){
    uint32_t tt =0;
    uint8_t tc =0;
    adc_init(); wait100ms(1);
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
    DBG("@get_power\r\n")
    #define POWER_SAMPLES 5
    #define POWER_ADC_CHAN 1
    uint32_t tmin=0, tmax =0;
    uint8_t tc =0;
    adc_init(); wait100ms(1);

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


/*
 * Use ADC to measure battery level
 */
uint16_t get_batteryLevel(void){
    uint8_t i,j;
    uint32_t adc[9];
    for(j=0;j<9;j++) adc[j] = 0;
    adc_init(); wait100ms(1);
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

// Hardware deep sleep. 
void hibernate(uint8_t p_sec, uint8_t p_isdeep)
{

    DBG("hibernate: go_sleep (deep_sleep=%u) for %d secs\r\n", p_isdeep, p_sec)
    rtc_init_osc(0);
    wait100ms(1); // little wait to let the system rest (flush serial ?)

    if (p_isdeep){
        // Hibernate (See sleep example of libmc1322x for details
        *CRM_WU_CNTL = 0x1;
        *CRM_WU_TIMEOUT =  p_sec * 2000; /* 60s */
        *CRM_SLEEP_CNTL = 0x71;
        while((*CRM_STATUS & 0x1) == 0) { continue; }
        *CRM_STATUS = 1;
        // Sleep
        while((*CRM_STATUS & 0x1) == 0) { continue; }
        *CRM_STATUS = 1;
    } else {
            wait100ms(SLEEP_DELAY * 10); // Simple CPU Loop
    }



    wait100ms(20); // Wait for hardware to wakeup properly (2sec ??? FIXME: find correct value)
    DBG("hibernate: wake_up (deep_sleep=%u) done\r\n", p_isdeep)
}




void reset_watchdog(void)
{
    DBG("@reset_watchdog\r\n")
    wait100ms(1); // little wait to let the system rest (flush serial ?)

    cop_timeout_ms(1);
    CRM->COP_CNTLbits.COP_EN = 1;
    while (1) continue;
}


void rad_init(void)
{
    // Init Radio
    DBG("# m:radio_init(chan: 0x%02x tx_pw: 0x%02X)\r\n" , myconfig.radiochan, myconfig.txpower);    
    maca_init();
    set_channel(myconfig.radiochan);
    set_power(myconfig.txpower);
}

void rad_on(void){
    DBG("# m:radio_on()\r\n");    
    maca_on(); 
    //check_maca();

}

void rad_off(void)
{
    // Init Radio
    DBG("# m:radio_off()\r\n");    
    wait100ms(1); // Time to flush Tx buffers
    maca_off();
}














/*
 * Main
 */
void main(void) {

    volatile packet_t *txp;
    volatile packet_t *rxp;

    tmr_cntr=0;
    cntr_msg_sent=0;
    awake_sec=0;      //
    main_loop_count=0; // #time we did the main look 

    // Wall clock
    clock_h=0;
    clock_m=0;
    clock_s=0;

    // Prepare timers.
    timers_init();
    enable_irq(TMR);

    // UART
    if (DODEBUG) uart_init(UART1, SERIAL_SPEED);
    DBG("#m: OFLnode start\r\n");

    // uController init
    trim_xtal();
    vreg_init();

    // Read config from NVM if possible
    DBG("# m:try to read config from NVM\r\n");
    int res;
    res=read_config(&myconfig);
    if ( res < 0) { // Problem reading configuration
        DBG("! m:error_reading_conf (%d)\r\n",res);

        if ( res == -3){ // No configuration signature in NVR
			default_config(&myconfig);
			memset(myconfig.smac,0,8);
			myconfig.smac[7]=NODE_ID;  						// Patch default config with NODE_NO 
			myconfig.span[0]=0x0D; myconfig.span[1]=0xF0;  	// Patch default config with PAN F00D

            res=write_config(&myconfig);
            if (res<0) {
                DBG("! m:error_wr_cnf_nvm\r\n");
            } else 
               DBG("# m:default_conf_write_ok\r\n");

        }
    }

/*
TODO: rewrite this shit

    if (myconfig.low_uptime_flag==0x01){  // NVM tells us previous uptime was short
        myconfig.low_uptime_counter++;    
        if (myconfig.low_uptime_counter> RESET_REBOOT_COUNT) state_init=1;  // Too much ON/OFF/ON/OFF short cycles
    } else  {
        myconfig.low_uptime_counter=0;   // The device 
    }


    if(state_init==0){ // Update counters only if not already in init mode
      DBG("# m:state_init=0\n");

        int z; 
	   for (z=0;z<10;z++){ // Blink 20x to notify for NVR config reset
            digitalWrite(RX_LED_PIN, 1); wait100ms(2);
            digitalWrite(RX_LED_PIN, 0); wait100ms(2);
	   }
        myconfig.low_uptime_flag=0x1;
        res=write_config(&myconfig);
    }
    else { 
        DBG ("Reset configuration and write to NVR");
    }
*/
    dump_config(myconfig);


    //RX TX LED
    // FIXME: Write a generic Blink fct
    setPinGpio(RX_LED_PIN, GPIO_DIR_OUTPUT);
    setPinGpio(TX_LED_PIN, GPIO_DIR_OUTPUT);


    int report_err;
    // Main loop

    // Init radio
    rad_init();

	// Detect bme280 and read calibration if needed
	uint8_t bme280_presence; 
	i2c_enable();
 	bme280_presence=detect_bme();
	if (bme280_presence) read_calibration_data(); 
	i2c_disable(); 

    while(1){

        rad_on();

        memset(tx_paquet.data,0,PAQUET_MAX_DATASIZE); // Zero TX message buffer 
        report_err=0; 

		// Early ds1820 power so it's operationnal for measurement later (if no BME280 detected)
        if ( !bme280_presence && myconfig.capa[0] & CAPA_TEMP ){
			DBG("!!! Enabling ds1820\r\n");
            ds1820_start();
		}

        // Add battery information every 10 cycles
        if ( main_loop_count==0 ||  (main_loop_count%5)==0)  {
            DBG("# m:Processing battery Level\r\n");
            uint16_t t_batl= get_batteryLevel();
            sprintf((char*)tx_paquet.data + strlen((char*)tx_paquet.data), "FWVER:%04d;CAPA:%02X%02X;BATLEV:%u;AWAKE_SEC:%lu;MAIN_LOOP:%lu;", 
                    FW_VER, myconfig.capa[1], myconfig.capa[0], t_batl, awake_sec , main_loop_count );
        }

        // Add temperature information if node has capability
        if ( !bme280_presence && myconfig.capa[0] & CAPA_TEMP ) 
        {
            DBG("# m:Processing CAPA_TEMP\r\n");
            uint8_t  m_cel, m_cel_frac, m_cel_sign;
            if (ds1820_readTemp(&m_cel_sign, &m_cel, &m_cel_frac))
            {
                if (m_cel == 85){
                    DBG("! m: CAPA_TEMP value is 85, skipping value\r\n");
                    report_err=11; 
                } else {
                    sprintf((char*)tx_paquet.data + strlen((char*)tx_paquet.data), "TEMP:%c%d.%02d;",
                        m_cel_sign == 1 ? '-' : '+',
                        m_cel, m_cel_frac);
                }

            } else {
                DBG("! m: ds1820_readTemp Error\r\n")
                report_err=12; 
            }
            ds1820_stop(); // Don't forget to shut down the ds1820 
        }

        if ( bme280_presence){
			int16_t temp=0;
			uint16_t humi=0;
			uint16_t pres=0;
            i2c_enable();
            temp = bmx280_read_temperature();
            pres = bmx280_read_pressure();
            humi = bme280_read_humidity();
            i2c_disable();
			sprintf((char*)tx_paquet.data + strlen((char*)tx_paquet.data), "TEMP:%c%d.%02d;PRESS:%04d;HUMI:+%d.%02d;", 
                        temp > 0 ? '+' : '-',
                        temp/100, temp%100,
						pres,humi/100, humi%100); 
		} 



        // Add light level information if node has capability
        if ( myconfig.capa[0] & CAPA_LIGHT ) {
            DBG("Processing CAPA_LIGHT\r\n");
            uint16_t m_lumi;
            if (get_lumi(&m_lumi) == 1){
                sprintf((char*)tx_paquet.data + strlen((char*)tx_paquet.data), "LUMI:+%05u;",m_lumi);
            }
        }

        // Add power information if node has capability
        if ( myconfig.capa[0] & CAPA_POWER ) {
            DBG("Processing CAPA_POWER\r\n");
            adc_init();
            uint16_t m_power;
            if ( get_power(&m_power) == 1 ){
                sprintf((char*)tx_paquet.data + strlen((char*)tx_paquet.data), "POWER:+%05u;",m_power);

            }
        }

        // Handle error reporting
        if (report_err>0) sprintf((char*)tx_paquet.data + strlen((char*)tx_paquet.data), "ERROR:%04d;",report_err);
 
        // Compute data length
        tx_paquet.datalen = strlen((char*)tx_paquet.data);

        // Time to RX
        check_maca();
        while (( rxp = rx_packet()))
        {
            //DBG("Packet received\r\n");
/*            process_rx_packet(rxp);
            // Just ignore the RX Packets for now 
*/
            free_packet(rxp);
        }

        // Send packet data
        txp = get_free_packet();
        if (txp == NULL) {
            DBG("! m:No radio free_packets...\r\n");
            goto the_end;
        } else {
            DBG("# m: TX Payload: '%s'\r\n", tx_paquet.data);
			tx_paquet.dpan[0]=0x0D;tx_paquet.dpan[1]=0xF0;			// Dest PAN is 0xF00D
			tx_paquet.span[0]=0x0D;tx_paquet.span[1]=0xF0;			// Dest PAN is 0xF00D
			memset(tx_paquet.smac,0x00,8);tx_paquet.smac[0]=NODE_ID;	// Dest MAC  is  00:00:00:00:00:00:00:<nodeid>
			memset(tx_paquet.dmac,0x00,8);tx_paquet.dmac[0]=0x01;		// Dest MAC  is  00:00:00:00:00:00:00:01


            if (paquet2packet( &tx_paquet,txp)){
				packet_dump(txp); 
                tx_packet(txp);
                cntr_msg_sent++;
            }
        }

        rad_off();

        /* FIXME: WIP: Reset to factory configuation after x consecutive reboots feature.
	       Basic idea: low_uptime_flag is used to determine if previous uptime
           was below defined threshold. If current run last more than the threshold, we have
           to clear the flag

        if (myconfig.low_uptime_flag==0x01 &&  tmr_cntr > 200 ){
            DBG("# m:Clean low_uptime_counter flag\r\n");
            myconfig.low_uptime_flag=0x00; 
            write_config(&myconfig);
        }
        */

        /* 
        Reseting the device every 12h (through watchdog)
        to fix unknown mcu/radio/code bug
        (oflnode become mute after hours/days of uptime) 
        10h = 10 * 60 * 60 
        */
        if ( (main_loop_count * SLEEP_DELAY) > (10 * 60 * 60) ) {
            DBG("Time to reset watchdog...\r\n")
            reset_watchdog();
        }



the_end:
        // Blink LED
        digitalWrite(RX_LED_PIN, 1);
        wait100ms(1);
        digitalWrite(RX_LED_PIN, 0);

        main_loop_count++;
        hibernate(SLEEP_DELAY, DEEP_SLEEP);

    }

}


/*
# Copyright (C) 2013, 2014 Olivier Fauchon
# This file is part of OFLMetrics.
#
# OFLMetrics is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# OFLMetrics is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
*/

#include <mc1322x.h>
#include <board.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gpio.h>

#include "libs/config.h"
#include "libs/utils.h"
#include "libs/utils-paquet.h"
#include "libs/xtea.h"


// Configuration defines
#define LED_RED_GPIO 50
#define LED_GREEN_GPIO 43

#define SLEEP_CYCLE 60
#define RADIO_POWER 0x12
#define RADIO_CHANNEL 0X00
#define NODE_NO 0X20
#define SERIAL_SPEED 115200


// Code defines
#define DUMP_HUMAN 1
#define DUMP_HEX 2
#define CMD_MAXLEN 255


#define MIN_CHAN 0 
#define MAX_CHAN 15 


//uint8_t SMAC[4] = { 0x00, 0x00, 0x00, 0x01 };
uint8_t BROADCAST_MAC[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
volatile uint8_t cur_chan, hop_spd;
volatile uint8_t monitor_mode;
volatile packet_t *p;
config_t myconfig; 


volatile uint8_t clock_d,clock_h,clock_m,clock_s, clock_100ms;
volatile uint32_t cntr;


// Just blink once
void bip_led(int LED_GPIO)
{
  #define PDELAY 500000
  volatile uint32_t i; 
  gpio_data_set(1ULL<< LED_GPIO);
  for(i=0; i<PDELAY; i++) { continue; }
  gpio_data_reset(1ULL<< LED_GPIO);
  for(i=0; i<PDELAY; i++) { continue; }
}

#define bip_led_red() bip_led(LED_RED_GPIO);
#define bip_led_green() bip_led(LED_GREEN_GPIO);




/*
 * selftest xtea encoding/decoding
 */
void selftest_xtea(void)
{
    char message[100];
    memset(message,0,sizeof(message) );
    sprintf(message,"Ceci est un test");

    uint8_t cipher_key[16] = "0123456789ABCDEF";

    XteaStateKey_T skey;
    XteaSetup(cipher_key, NULL, &skey);

    uint8_t len= strlen(message);
    uint8_t cnt; 
    printf("plain:    ");for (cnt=0; cnt<len; cnt++) { printf("%02X'%c' ", message[cnt], message[cnt]);}printf("\r\n");

    XteaEncrypt((uint8_t*) message,(uint8_t*) message, len, &skey);
    printf("crypted:  ");for (cnt=0; cnt<len; cnt++) { printf("%02X'%c' ", message[cnt], message[cnt]);} printf("\r\n");    

    XteaRestart(&skey);
    XteaDecrypt((uint8_t*)message,(uint8_t*)message,len,&skey);
    printf("decrypted:");for (cnt=0; cnt<len; cnt++) { printf("%02X'%c' ", message[cnt], message[cnt]);} printf("\r\n");    
}


/*
 * Dumps a packet in a human-readable form
 */
void dump_human(volatile packet_t *p, paquet *pk) 
{ 
    p=p;
    printf("RX: %02X%02X%02X%02X|%02X%02X%02X%02X|%02X|%s|\r\n",
           pk->smac[0],pk->smac[1],pk->smac[2],pk->smac[3],
           pk->dmac[0],pk->dmac[1],pk->dmac[2],pk->dmac[3],
           pk->datalen, pk->data);
}

/*
 * Dumps a packet in binary form
 */
void dump_hex(volatile packet_t *p, paquet *pk)
{ 
    volatile uint8_t idx_char,idx_ligne,offset;
     #define PER_ROW 16
    char sss[PER_ROW+1];
    if(p) {
        printf("RX: len=0x%02x(%d) lqi=0x%02x(%d) rx_time=0x%08x(%d) smac=%02X%02X%02X%02X dmac=%02X%02X%02X%02X len=%d\r\n",
               p->length,p->length, p->lqi,p->lqi, (int)p->rx_time, (int)p->rx_time,
               pk->smac[0],pk->smac[1],pk->smac[2],pk->smac[3],
               pk->dmac[0],pk->dmac[1],pk->dmac[2],pk->dmac[3],
               pk->datalen);

        char c;
        // Ligne complete
        for(idx_ligne=0, offset=0; idx_ligne <= ( (p->length) / PER_ROW ); idx_ligne++) {
            memset(sss, 0, PER_ROW+1);
            printf("~ ");

            // Chaque octet:
            for(idx_char=0; idx_char < PER_ROW; idx_char++, offset++) {
                // On dépasse la longueur du paket...
                if(offset >= p->length ) {
                    int z;for (z=idx_char+1; z < PER_ROW; z++) { printf("   ");} // pour aligner la derniere ligne
                    printf("   %s\r\n",sss);
                    return;
                }

                c=p->data[idx_ligne*PER_ROW + idx_char + p->offset] ;
                if (c >= ' ' && c <= '~') {
                    sss[idx_char] =c;
                } else sss[idx_char]='.';
                // Hex
                printf("%02x ",c);
            } // Fin ligne
            printf("%s\r\n",sss);
        }
        printf("\r\n");
    }
    return;
}



/*
 * RX Callback
 */
void maca_rx_callback(volatile packet_t *p) {
    (void)p;
    bip_led_red(); 
}


/*
 * Here we process the user commands
 */
void process_cmd(char* cmd)
{

    // Stop Monitor Mode if empty command
    if (monitor_mode != 0 && cmd[0] == 0x0 ) { //Empty command
        printf("packet-dump stopped.\r\n");
        monitor_mode=0;
    }
    // Help
    else if (strncmp(cmd,"?",1) ==0 )
    {
        printf(":dump_hex        Dump incoming packets (hex form)\r\n");
        printf(":dump_human      Dump incoming packets (human-readable)\r\n");
        printf(":show_config     Dump current config\r\n");
        printf(":channel XX      Force radio channel to XX\r\n");
        printf(":chan_hop XX     Force channel hopping every 5<XX<180 seconds\r\n");
        printf(":selftest_xtea   Test XTEA Encoding/Decoding\r\n");
        printf(":info            Informations\r\n");
        printf(":send MY_COMMAND      Send the command\r\n");
    }
    // Hexa dump
    else if (strstr(cmd,":dump_hex")) {
        monitor_mode=DUMP_HEX;
        printf("OK dump_hex mode <press any key to stop>\r\n");
    }
    // Human dump 
    else if (strstr(cmd,":dump_human")) {
        monitor_mode=DUMP_HUMAN;
        printf("OK dump_human mode <press any key to stop>\r\n");
    }
    else if ( strstr(cmd, ":channel ") && strlen(cmd) > 9 ) {
        int ch = atoi (cmd+9);
        set_channel(ch); /* channel 11 */
	cur_chan=ch; 
        printf("OK channel set to %d", cur_chan);
    }
    else if ( strstr(cmd, ":chanhop ") && strlen(cmd) > 9 ) {
        int tmp = atoi (cmd+9);
	if ( tmp > 5 && tmp < 180) hop_spd=tmp;
        printf("OK channel hopping every %d s", hop_spd);
    }
    else if ( strstr(cmd, ":selftest_xtea")) {
        printf("OK running selftest_xtea\r\n");
            selftest_xtea();
    }
    else if ( strstr(cmd, ":show_config")) {
        printf("OK showing configuration\r\n");
            dump_config(myconfig);
    }
    else if ( strstr(cmd,":send ")){
        char* cmd2= (char*) (cmd+6);
        printf("TX : '%s', size %d\r\n", cmd, strlen(cmd2));
        p = get_free_packet();
        if (p != NULL ){
            paquet pq;
            memcpy (pq.smac, myconfig.smac, 4);
            memcpy (pq.dmac, BROADCAST_MAC, 4);
            pq.datalen=strlen(cmd2);
            memcpy(pq.data, cmd2, strlen(cmd2));
            pq.crc[0]=0;
            pq.crc[1]=0;
            if (paquet2packet(&pq,p))
            {
                dump_hex(p,&pq);
                tx_packet(p);
            } else printf("ERR: Can't create paquet_t\r\n");   
        } else {
            printf("ERR: No free TX packets\r\n");
        }


    } else {
	printf("ERROR: Unknown command\r\n");
    }

}

void kbi4_isr(void){
	printf("O\n");
	clear_kbi_evnt(4);
}
void kbi7_isr(void){
	printf("N\n");
	clear_kbi_evnt(7);
}


void init_hw(void)
{
    // Set KBI6(GPIO28) as input
    #define PIN_KBI6 28
 //   setPinGpio(PIN_KBI6, GPIO_DIR_INPUT);
 //   gpio_sel0_pullup(PIN_KBI6); 

    enable_irq_kbi(7);
    kbi_edge(7); 	//KBI_7 is edge sensitive (in opposite to level sensitive)
    enable_ext_wu(7);	//KBI_7 is configured to wake up cpu from wakeup or doze

    enable_irq_kbi(4);
    kbi_edge(4); 	//KBI_4 is edge sensitive (in opposite to level sensitive)
    enable_ext_wu(4);	//KBI_4 is configured to wake up cpu from wakeup or doze

    // Configure RED & GREEN LEDs GPIO
    setPinGpio(LED_RED_GPIO, GPIO_DIR_OUTPUT);
    setPinGpio(LED_GREEN_GPIO, GPIO_DIR_OUTPUT);


    // Bling GREEN
    gpio_data_set(1ULL<< LED_GREEN_GPIO);
    gpio_data_reset(1ULL<< LED_GREEN_GPIO);
    // trim the reference osc. to 24MHz , deal with voltage regulator
    trim_xtal();
    vreg_init();
    // Init Serial port 1 
    uart_init(UART1, SERIAL_SPEED);
    // Init Radio
    maca_init();
    set_channel(myconfig.radiochan);
    set_power(myconfig.txpower);


}














/*
 *  TIMER STUFF START
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

    if ( hop_spd>0 && (clock_s % hop_spd )==0) // Time to hop ! 
    {
	cur_chan++; 
	if (cur_chan>MAX_CHAN) cur_chan=0;
	set_channel(cur_chan); 
	printf("Channel hopping to ch:%d\r\n",cur_chan); 

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
 *  TIMER STUFF END 
 */ 















// TODO: Enable timers and finish channel hopping




/*
 * THE MAIN 
 *
 */
void main(void) 
{

    init_hw();
    bip_led_green(); 
    bip_led_red(); 
    printf("OFLdongle start\r\n");

    paquet pk;
    paquet *ppk = &pk;
    char cmd_data[CMD_MAXLEN+1];
    uint8_t cmd_pos=0;
    monitor_mode=0;
    hop_spd=0; // No channel hopping at start 

    // Wall clock
    clock_h=0;
    clock_m=0;
    clock_s=0;

    // Prepare timers.
//    timers_init();

    // CRM is needed for KBI interrupts 
    enable_irq(CRM);


    // Get config
    uint8_t ret= read_config(&myconfig);
    if (ret!=1) { printf("   Failed, using defaults\r\n"); default_config(&myconfig); }

    memset(cmd_data,0,CMD_MAXLEN+1); // Zero the command buffer

    while(1) {

        /* call check_maca() periodically --- this works around */
        /* a few lockup conditions */
        check_maca();

        // Check packet.
        if((p = rx_packet()))
        {
            packet2paquet(p, ppk);
            /* print and free the packet */
            if (monitor_mode==DUMP_HUMAN) dump_human(p , ppk);
            else if (monitor_mode==DUMP_HEX) dump_hex(p , ppk);
            free_packet(p);
        }

        // Read serial port
        if(uart1_can_get())
        {
            char c = uart1_getc();
            // Buffer full
            if (cmd_pos == CMD_MAXLEN) {
                printf("Command too long\r\n");
                cmd_pos=0;
                memset(cmd_data,0,CMD_MAXLEN);
            }
            // New line received, process the command
            else if (c == 0x0D || c == 0x0A) {
                printf("\r\n");
                process_cmd(cmd_data);
                cmd_pos=0;
                memset(cmd_data,0,CMD_MAXLEN);
            }
            // Append new char to command 
            else {
                printf("%c",c);
                cmd_data[cmd_pos++]=c;
            }



        }

    }
}

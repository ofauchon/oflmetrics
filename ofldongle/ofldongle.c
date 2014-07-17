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


//uint8_t SMAC[4] = { 0x00, 0x00, 0x00, 0x01 };
uint8_t BROADCAST_MAC[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
volatile uint8_t chan;
volatile uint8_t monitor_mode;
volatile packet_t *p;
config_t myconfig; 


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
    printf(">>> %02X%02X%02X%02X|%02X%02X%02X%02X|%02X|%s|\r\n",
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
        printf(">>> len=0x%02x(%d) lqi=0x%02x(%d) rx_time=0x%08x(%d) smac=%02X%02X%02X%02X dmac=%02X%02X%02X%02X len=%d\r\n",
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
    gpio_data_set(1ULL<< LED_RED_GPIO);
    gpio_data_reset(1ULL<< LED_RED_GPIO);
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
        printf(":dump_hex Dump incoming packets as hex\r\n");
        printf(":dump_human Dump incoming in readable parsable form\r\n");
        printf(":dump_config         Dump current config\r\n");
        printf(":channel XX          Set radio chan\r\n");
        printf(":selftest_xtea           Test XTEA Encoding/Decoding\r\n");
        printf(">MY_COMMAND          Send the command\r\n");
    }
    // Hexa dump
    else if (strstr(cmd,":dump_hex")) {
        monitor_mode=DUMP_HEX;
        printf("dump_hex enabled. any key to stop\r\n");
    }
    // Human dump 
    else if (strstr(cmd,":dump_human")) {
        monitor_mode=DUMP_HUMAN;
        printf("dump_human enabled. any key to stop\r\n");
    }
    else if ( strstr(cmd, ":channel ") && strlen(cmd) > 9 ) {
        int ch = atoi (cmd+9);
        set_channel(ch); /* channel 11 */
        printf("radio chan now : %d", ch);
    }
    else if ( strstr(cmd, ":selftest_xtea")) {
            selftest_xtea();
    }
    else if ( strstr(cmd, ":dump_config")) {
            dump_config(myconfig);
    }
    else if ( strlen(cmd) >0 && cmd[0]=='>'){
        char* cmd2= (char*) (cmd+1);
        printf("send command : '%s', size %d\r\n", cmd, strlen(cmd2));
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


    }

}



void init_hw(void)
{
    // Configure LED GPIOs
    setPinGpio(LED_RED_GPIO, GPIO_DIR_OUTPUT);
    setPinGpio(LED_GREEN_GPIO, GPIO_DIR_OUTPUT);
    gpio_data_set(1ULL<< LED_RED_GPIO);
    gpio_data_reset(1ULL<< LED_GREEN_GPIO);

    /* trim the reference osc. to 24MHz , deal with voltage regulator*/
    trim_xtal();
    vreg_init();
    // Init Serial port 1 
    uart_init(UART1, SERIAL_SPEED);
    // Init Radio
    maca_init();
    set_channel(myconfig.radiochan);
    set_power(myconfig.txpower);
}

void config_default_values(config_t *myconfig)
{
    myconfig->smac[0]=0x01;myconfig->smac[1]=0x02;myconfig->smac[2]=0x03;myconfig->smac[3]=0x04;
    myconfig->txpower=0x12;
    myconfig->radiochan=0x00;
    myconfig->capa[0]=0xFF;myconfig->capa[1]=0xFF;
    myconfig->signature[0]=0xF0; myconfig->signature[1]=0x0F; 
    myconfig->low_uptime_counter=0; 
}


/*
 * THE MAIN 
 *
 */
void main(void) 
{
    init_hw();
    printf("OFLdongle start\r\n");

    paquet pk;
    paquet *ppk = &pk;
    char cmd_data[CMD_MAXLEN+1];
    uint8_t cmd_pos=0;
    monitor_mode=0;

    // Get config
    printf("Reading NVRam configuration\r\n");
    uint8_t ret= read_config(&myconfig);
    if (ret!=1) { printf("   Failed, using defaults\r\n"); config_default_values(&myconfig); }



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
            if (cmd_pos == CMD_MAXLEN) {
                printf("Command too long\r\n");
                cmd_pos=0;
                memset(cmd_data,0,CMD_MAXLEN);
            }
            // New line received, process the command
            else if (c == 0x0D || c == 0x0A) {
                printf("%c",c);
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

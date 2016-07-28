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
#include <put.h>

#include "libs/config.h"
#include "libs/utils.h"
#include "libs/ringbuffer.h"

// Configuration defines
#define LED_RED_GPIO 50
#define LED_GREEN_GPIO 43
#define KBI4_GPIO 26
#define KBI7_GPIO 29
#define SERIAL_SPEED 115200

#define PRECISION 50

// Code defines
#define CMD_MAXLEN 255

#define us2cnt(A) ((1875*A)/10000)
#define cnt2us(A) ((10000*A)/1875)

volatile uint8_t clock_d,clock_h,clock_m,clock_s, clock_100ms;
volatile uint8_t kbi7_val;

volatile uint16_t delta;
volatile uint8_t record;
ringBufS ring;
volatile uint8_t dbg=0; 

char msg_buf[1000];
uint16_t msg_cnt; 

int16_t trans_prev, trans_cur; 
volatile uint8_t b_edge_direction; 

// Helper function find boundaries or valid tolerence range 
#define LOLO(x,y) ((x*(100-y))/100)
#define HIHI(x,y) ((x*(100+y))/100)
#define HE_BITLEN 64

/*
 * ring buffer testing
 *
void test_ring(void){
	ringBufS ri; 
    ring_init(&ri,40); 
	printf("ri_size: %d\r\n", ri.size); 
	int i; 
	//for (i=0; i<30; i++){ printf("  value  pos:%d real: %d: should_be:0 \r\n", i, ring_get(&ri, i)); }
	//printf("ri_size:%d ri_head:%d ri_tail:%d\r\n", ri.size, ri.head, ri.tail); 
	printf("* push 30 val (101-130)\r\n"); for (i=1; i<=30; i++){ ring_push(&ri, 100+i);}
	printf("head_len:%d head_cnt:%d head_pos:%d tail_pos:%d\r\n", ri.size, ri.count, ri.head, ri.tail); 
	for (i=0; i<ri.count; i++){ printf("%d ", ring_get(&ri, i)); if ((i%10)==0) printf("\r\n");  } printf("\r\n");

	printf("* push 30 val (201-230)\r\n"); for (i=1; i<=30; i++){ ring_push(&ri, 200+i);}
	printf("head_len:%d head_cnt:%d head_pos:%d tail_pos:%d\r\n", ri.size, ri.count, ri.head, ri.tail); 
	for (i=0; i<ri.count; i++){ printf("%d ", ring_get(&ri, i)); if ((i%10)==0) printf("\r\n");  } printf("\r\n");

	printf("* Remove 10 from queue\r\n"); ring_remove_tail(&ri, 10); 
	printf("head_len:%d head_cnt:%d head_pos:%d tail_pos:%d\r\n", ri.size, ri.count, ri.head, ri.tail); 
	for (i=0; i<ri.count; i++){ printf("%d ", ring_get(&ri, i)); if ((i%10)==0) printf("\r\n");  } printf("\r\n");
}
*/

/*
 * Here we process the user commands
 */
void process_cmd(char* cmd)
{
    if (cmd[0]=='?')
    {
        putstr(":help            Display help\r\n");
        putstr(":state           Dump internal state\r\n");
        putstr(":dbg           Dump internal state\r\n");
        putstr(":selftest        Self-test the unit\r\n");
        putstr(":showconfig      Display config\r\n");
        putstr(":info      Display informations\r\n");
    }
    // Hexa dump
    else if (strstr(cmd,":state")) {
        putstr("OK State informations\r\n");
    }
    else if (strstr(cmd,":dbg")) {
        if (dbg) {putstr("OK No Debug Disabled\r\n"); dbg=0;}
        else if (!dbg) {putstr("OK Debug Enabled\r\n"); dbg=1;}
    }
    else if (strstr(cmd,":selftest")) {
        putstr("OK selftest\r\n"); 
		//test_ring(); 
    }
    else if (strstr(cmd,":info")) {
		char tmp[10]; 
        putstr("OK info\r\n"); 
		putstr("Range for 275: "); putstr(my_itoa(LOLO(275,PRECISION),tmp)); putchr('>'); putstr(my_itoa(HIHI(275,PRECISION),tmp)); putstr("\r\n");
		putstr("Range for 1225: "); putstr(my_itoa(LOLO(1225,PRECISION),tmp));putchr('>');  putstr(my_itoa(HIHI(1225,PRECISION),tmp)); putstr("\r\n");
    }
    else if ( strstr(cmd, ":show_config")) {
        putstr("OK showing configuration\r\n");
    } else {
	putstr("ERROR: Unknown command\r\n");
    }

}

/*
// check if value is near ref, plus/minus precision
int is_around(int16_t val, int16_t ref, int16_t precision)
{
	uint8_t ret=0; 
	int16_t min = ( (ref * (100-precision)) /100); 
	int16_t max = ( (ref * (100+precision)) /100);
	if (ref>0 && val>min && val<max) ret=1; 
	else if (ref<0 && val<min && val>max) ret=1; 
	return ret; 
}
*/


/*
 * This code is run every time a transition is pushed in the ring 
 */
void data_process(void)
{
	uint8_t i;
	int16_t d0,d1; 
	char r; 
   	for (i=0; i<HE_BITLEN ; i++){
		d0=ring_get(&ring, i*2  ); 
		d1=ring_get(&ring, i*2 + 1 ); 
		r='?'; 
		if ( d0>LOLO(275,PRECISION) && d0<HIHI(275,PRECISION) && d1>-(HIHI(275,PRECISION)) &&  d1<-(LOLO(275,PRECISION)) ) {r='1';}
		else if ( d0>LOLO(275,PRECISION) && d0<HIHI(275,PRECISION) && d1>-(HIHI(1225,PRECISION)) &&  d1<-(LOLO(1225,PRECISION)) ) {r='0';}

		msg_buf[ (100*msg_cnt) + i ]=r; 

		if (r=='?' && i>0 ) {
			char tmp[10]; 
			putstr("Error: "); putstr(my_itoa(d0,tmp)); putchr(','); putstr(my_itoa(d1,tmp)); putstr("\r\n");
		}
   	}
	msg_buf[ (100*msg_cnt) + i ]=0; // End the string 
	msg_cnt++; 
	// END HOMEASY

}

// http://blog.domadoo.fr/2010/03/21/principe-du-protocole-homeeasy/
// 0=> high 275uS and 2675us low  
// 1=> high 275uS and 1300us low  
/*
 * External interrupt procession
 */
char tmp[10];
void kbi7_isr(void)
{
	delta = *TMR1_CNTR;
	*TMR1_CNTR=0; 

	trans_prev=trans_cur; 

    // Change edge direction
    if (b_edge_direction==0){
        b_edge_direction=1;
        kbi_pol_pos(7);
		trans_cur=+cnt2us(delta); 
    }
    else if (b_edge_direction==1){
        b_edge_direction=0;
        kbi_pol_neg(7);
		trans_cur=-cnt2us(delta); 
    }
    // If we find transitions, then start recording with countdown 
	if ( record == 0 ){
		// Search start pattern low 10ms, high 275uS, low 2675uS
		if (trans_prev>LOLO(275,PRECISION) && trans_prev<HIHI(275,PRECISION) && trans_cur>(-(HIHI(2675,PRECISION))) &&  trans_cur<(-(LOLO(2675,PRECISION))) ) {
			record=HE_BITLEN*2;  // Record countdown
			//putstr("Latches\r\n"); 
		}
	}
	if (record > 0 )  {  // record != 0 , we are recording 
	//	putstr(my_itoa(trans_cur,tmp)); putchr(',');
		ring_push(&ring, trans_cur); 
		record--; 
	}
	if ( record == 1 ) {
		//putstr("data_process\r\n");
		data_process();  // We process the ring only once to save resources. 
		ring_reset(&ring);
	}
	
	clear_kbi_evnt(7);
}




/*
 * Hardware initialisation
 */
void init_hw(void)
{
    // Enable extenal interrupt (KBI7)
    enable_irq_kbi(7);
    kbi_edge(7);        // Edge mode 
    kbi_pol_pos(7);     // Default edge direction Detection 
    enable_ext_wu(7);	// wake up cpu from wakeup or doze ???

    // Led Pin
    setPinGpio(LED_RED_GPIO, GPIO_DIR_OUTPUT);
    setPinGpio(LED_GREEN_GPIO, GPIO_DIR_OUTPUT);

    trim_xtal();
    vreg_init();
    uart_init(UART1, SERIAL_SPEED);
}


/*
 *  TIMER0 => Wall clock timer
 */ 
void tmr0_isr(void)
{   
    // Count time 
    clock_100ms+=1;
    if (clock_100ms == 10)  { clock_s++; clock_100ms=0; }
    if (clock_s == 60)      { clock_m++; clock_s=0;}
    if (clock_m == 60)      { clock_h++; clock_m=0;}
    if (clock_h == 24)      { clock_d++; clock_h=0;}

    // Led flash every second (300ms) 
    if ( ((clock_s % 5==0)) &&  clock_100ms<3) {
        gpio_data_set(1ULL<< LED_RED_GPIO);
	} else {
        gpio_data_reset(1ULL<< LED_RED_GPIO);
    }

    // Restart counter
    *TMR0_SCTRL = 0;
    *TMR0_CSCTRL = 0x0040; // clear compare flag 

}


/*
 * Timer initialization
 */ 
void timers_init(void)
{   
    // TMRX_CTRL : page 12-13
    // TMRX_SCTRL : page 12-14
    // TMRX_CSCTRL : page 12-19
    #define TT1_COUNT_MODE 1      /* use rising edge of primary source P12-14 */
    #define TT1_PRIME_SRC  0xf    /* Perip. clock with 128 prescale (for 24Mhz = 187500Hz) P12-14 */
    #define TT1_SEC_SRC    0      /* don't need this */
    #define TT1_ONCE       0      /* keep counting */
    #define TT1_LEN        1      /* count until compare then reload with value in LOAD */
    #define TT1_DIR        0      /* count up */
    #define TT1_CO_INIT    0      /* other counters cannot force a re-initialization of this counter */
    #define TT1_OUT_MODE   0      /* OFLAG is asserted while counter is active */

    *TMR_ENBL     = 0;                    /* tmrs reset to enabled */

    // Timer0 => Internal wall clock time 
    *TMR0_SCTRL   = 0;
    *TMR0_CSCTRL  = 0x0040;               /* Enable int when TCF1 flag is ON. TCF1 flag is set when counter==COMP1 */
    *TMR0_LOAD    = 0;                    /* reload to zero */
    *TMR0_COMP_UP = 18750;                /* trigger a reload at the end */
    *TMR0_CMPLD1  = 18750;                /* compare 1 triggered reload level, 10HZ maybe? */
    *TMR0_CNTR    = 0;                    /* reset count register */
    *TMR0_CTRL    = (TT1_COUNT_MODE<<13) | (TT1_PRIME_SRC<<9) | (TT1_SEC_SRC<<7) | (TT1_ONCE<<6) | (TT1_LEN<<5) | (TT1_DIR<<4) | (TT1_CO_INIT<<3) | (TT1_OUT_MODE);

    #define TT2_COUNT_MODE 1      /* use rising edge of primary source P12-14 */
    #define TT2_PRIME_SRC  0xf    /* Perip. clock with 128 prescale (for 24Mhz = 187500Hz) P12-14 */
    #define TT2_SEC_SRC    0      /* don't need this */
    #define TT2_ONCE       0      /* keep counting */
    #define TT2_LEN        0      /* Count and rollover */
    #define TT2_DIR        0      /* count up */
    #define TT2_CO_INIT    0      /* other counters cannot force a re-initialization of this counter */
    #define TT2_OUT_MODE   0      /* OFLAG is asserted while counter is active */

    // Timer1 => Counter used to measuer transitions
    // No compare, no interrupt, no load. 
    // 24 Mhz with 128 prescale = 187500Hz. So 1 count = 5.333333... uS
    // 
    *TMR1_SCTRL   = 0;
    *TMR1_CSCTRL  = 0x0000;               /* No interrupt here, just count */
    *TMR1_LOAD    = 0;                    /* reload to zero */
    *TMR1_COMP_UP = 18750;                /* trigger a reload at the end */
    *TMR1_CMPLD1  = 18750;                /* compare 1 triggered reload level, 10HZ maybe? */
    *TMR1_CNTR    = 0;                    /* reset count register */
    *TMR1_CTRL    = (TT2_COUNT_MODE<<13) | (TT2_PRIME_SRC<<9) | (TT2_SEC_SRC<<7) | (TT2_ONCE<<6) | (TT2_LEN<<5) | (TT2_DIR<<4) | (TT2_CO_INIT<<3) | (TT2_OUT_MODE);

    *TMR_ENBL     = 0xf;                  /* enable all the timers --- why not? */

}




/*
 * THE MAIN 
 *
 */
void main(void) 
{

    init_hw();
    putstr("OFLomniradio start\r\n");

    // Wall clock
    clock_h=0;
    clock_m=0;
    clock_s=0;

	// Message processiong
    #define RINGS_SZ 1024
	if (ring_init(&ring, RINGS_SZ)==0)  putstr("Erreur allocation ring buffer\r\n"); 
	bzero(msg_buf, 1000);
	msg_cnt=0; 
	record=0; 

    // Prepare timers.
	timers_init();
	enable_irq(CRM); 
	enable_irq(TMR);

    // Input command buffer
    uint8_t cmd_pos=0;
    char cmd_data[CMD_MAXLEN+1];
    memset(cmd_data,0,CMD_MAXLEN+1); // Zero the command buffer

/*
    char tmp[10]; 
	int16_t r1; 
    r1=us2cnt(-12000);
	putchr('C'); putstr(my_itoa(r1,tmp));putstr("\r\n"); 
	putchr('M'); putstr(my_itoa(cnt2us(r1),tmp));putstr("\r\n"); 
*/

    while(1) {
		if (msg_cnt>0){
			int j; 
			for (j=0;j<msg_cnt;j++){
				putchr('A'+j); 
				putstr(msg_buf+(100*(j)) ); 
				putstr("\r\n"); 
				msg_cnt--; 
			}
		}

        // Read serial port
        if(uart1_can_get())
        {
            char c = uart1_getc();
            // Buffer full
            if (cmd_pos == CMD_MAXLEN) {
                putstr("Command too long\r\n");
                cmd_pos=0;
                memset(cmd_data,0,CMD_MAXLEN);
            }
            // New line received, process the command
            else if (c == 0x0D || c == 0x0A) {
                putstr("\r\n");
                process_cmd(cmd_data);
                cmd_pos=0;
                memset(cmd_data,0,CMD_MAXLEN);
            }
            // Append new char to command 
            else {
                putchr(c);
                cmd_data[cmd_pos++]=c;
            }



        }

    }
}

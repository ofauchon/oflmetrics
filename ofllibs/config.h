/*
 * Copyright (c) 2010, Mariano Alvira <mar@devl.org> and other contributors
 * to the MC1322x project (http://mc1322x.devl.org)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of libmc1322x: see http://mc1322x.devl.org
 * for details. 
 *
 *
 */

#ifndef CONFIG_H
#define CONFIG_H


// CONFIGURATION
#define DEFAULT_RADIOPOWER 0x12
#define DEFAULT_RADIOCHANNEL 0X00
#define DEFAULT_NODENO 0x26

#define FW_VER 337
#define TEMP_ENABLE  0
#define LIGHT_ENABLE 0
#define POWER_ENABLE 0

// #define ENABLE_HIBERNATE
#define HIBERNATE_DELAY 10
#define RX_LED_PIN 44
#define TX_LED_PIN 44
#define DODEBUG 1

#define NVM_CONFIG_POS 0x1e000
#define NVM_CONFIG_SZ 30


#define RESET_REBOOT_COUNT 5


/* Baud rate */
#define MOD 9999
/*  230400 bps, INC=767, MOD=9999, 24Mhz 16x samp */
/*  115200 bps, INC=767, MOD=9999, 24Mhz 8x samp */
#define INC 767  
/*  921600 bps, MOD=9999, 24Mhz 16x samp */
//#define INC 3071 
#define SAMP UCON_SAMP_8X
//#define SAMP UCON_SAMP_16X
//* sleep */
#undef USE_32KHZ /* board should have a HAS_32KHZ define */





#define DBG(A ...) if (DODEBUG) printf(A);

#endif

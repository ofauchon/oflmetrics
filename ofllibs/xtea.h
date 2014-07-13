#ifndef XTEA_H_
#define XTEA_H_
/* ================================================================================ *//**
 \license            ROBOOT ver. 0.1.0 - Copyright (C) 2010 AGH UST

           This file is part of the ROBOOT (RObus BOOTloader) project.
             ROBOOT is developed at the Department of Electronics,
           AGH University of Science and Technology in Krakow, Poland.

      The ROBOOT project uses dual-licensing. You can use it for free for 
   non-commercial, open source projects. See license.txt for details or visit 
           project home page at http://www.wsn.agh.edu.pl/?q=roboot
           
  THIS SOFTWARE IS PROVIDED 'AS IS'.  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
  OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
  AGH UST SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, 
  OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
  

 \file		  xtea.h
 \author      Artur Lipowski
 \purpose     ROBOOT
 \compiler    GCC
 \hardware    independent
 \doc         doxygen
 \brief       XTEA cipher API for ROBOOT
 \comments
*//* ================================================================================= */

#include <stdint.h>

#define XTEA_KEYLEN 	16
#define XTEA_BLOCKLEN	8

/* structure used internally by the XTEA algorithm implementation to keep actual state of encryption */
typedef struct {
	uint32_t A[32];
	uint32_t B[32];
	uint8_t IV[XTEA_BLOCKLEN];
	uint8_t pad[XTEA_BLOCKLEN];
	uint32_t padlen;
} XteaStateKey_T;

/**
 *  Initializes XTEA encryption algorithm API.
 *  Because actual data and state needed for encryption/decryption is stored into separate space (skey)
 *  then it is possible to perform many operations simultaneously.
 *  It is safe to call this function many times for given skey data.
 *  Function does not use dynamic memory allocation.
 *
 *  \param key	a key used for encryption and decryption
 *  \param iv	a initialization vector for algorithm (if NULL then it will be not used)
 *  \param skey	a pointer to internally used data
 */
void XteaSetup(const uint8_t *key, const uint8_t *iv, XteaStateKey_T *skey);

/**
 *  Reinitializes XTEA encryption algorithm API.
 *  This function can be used for subsequent encryption/decryption of data using the same key.
 *
 *  \param skey	a pointer to internally used data (it have to be initialized previously by XteaSetup function)
 */
void XteaRestart(XteaStateKey_T *skey);

/**
 *  Encrypts a block of text with XTEA algo.
 *
 *  \param pt 	the input plain text
 *  \param ct 	the output cipher text
 *  \param len 	the length of data for encryption
 *  \param skey	a pointer to internally used data
 */
void XteaEncrypt(const uint8_t *pt, uint8_t *ct, uint32_t len, XteaStateKey_T *skey);

/**
 *  Decrypts a block of text with XTEA algo.
 *
 *  \param ct 	the output cipher text
 *  \param pt 	the input plain text
 *  \param len 	the length of data for encryption
 *  \param skey	a pointer to internally used data
 */
void XteaDecrypt(const uint8_t *ct, uint8_t *pt, uint32_t len, XteaStateKey_T *skey);

#endif /*XTEA_H_*/

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
  

 \file		  xtea.c
 \author      Artur Lipowski
 \purpose     ROBOOT
 \compiler    GCC
 \hardware    independent
 \doc         doxygen
 \brief       XTEA cipher for ROBOOT
 \comments
*//* ================================================================================= */

#include "xtea.h"


#define STORE32L(x, y)                      \
	do {									\
		(y)[3] = (uint8_t)(((x)>>24)&255);	\
		(y)[2] = (uint8_t)(((x)>>16)&255);	\
		(y)[1] = (uint8_t)(((x)>>8)&255);	\
		(y)[0] = (uint8_t)((x)&255);		\
	} while (0)

#define LOAD32L(x, y)                           \
	do {										\
		(x) = ((uint32_t)((y)[3] & 255)<<24) |	\
              ((uint32_t)((y)[2] & 255)<<16) | 	\
              ((uint32_t)((y)[1] & 255)<<8)  | 	\
              ((uint32_t)((y)[0] & 255));		\
	} while (0)



static void xtea_encrypt(const uint8_t *pt, uint8_t *ct, XteaStateKey_T *skey);


/**************************************************************************************************\
\**************************************************************************************************/
void XteaSetup(const uint8_t *key, const uint8_t *iv, XteaStateKey_T *skey)
{
	uint32_t x;
	uint32_t sum;
	uint32_t K[4];

	//assert(key != NULL);
	//assert(skey != NULL);

	for(x = 0; x < XTEA_BLOCKLEN; x++)
	{
		if (iv)
		{
			skey->IV[x] = iv[x];
		}
		else
		{
			skey->IV[x] = x;
		}
	}

	/* load key */
	LOAD32L(K[0], key+0);
	LOAD32L(K[1], key+4);
	LOAD32L(K[2], key+8);
	LOAD32L(K[3], key+12);

	sum = 0;
	for (x = 0; x < 32; x++)
	{
		skey->A[x] = sum + K[sum & 3U];
		sum = sum + 0x9E3779B9UL;
		skey->B[x] = sum + K[(sum>>11) & 3U];
	}

	XteaRestart(skey);
}


/**************************************************************************************************\
\**************************************************************************************************/
void XteaRestart(XteaStateKey_T *skey)
{
	uint32_t x;

	//assert(skey != NULL);

	for(x = 0; x < XTEA_BLOCKLEN; x++)
	{
		skey->pad[x] = skey->IV[x];
	}

	skey->padlen = XTEA_BLOCKLEN;
}


/**************************************************************************************************\
\**************************************************************************************************/
void XteaEncrypt(const uint8_t *pt, uint8_t *ct, uint32_t len, XteaStateKey_T *skey)
{
	while (len)
	{
		//assert(pt != NULL);
		//assert(ct != NULL);
		//assert(skey != NULL);

		if (XTEA_BLOCKLEN == skey->padlen)
		{
			xtea_encrypt(skey->pad, skey->pad, skey);
			skey->padlen = 0;
		}
		*ct++ = *pt++ ^ skey->pad[skey->padlen++];
		len--;
	}
}

/**************************************************************************************************\
\**************************************************************************************************/
void XteaDecrypt(const uint8_t *ct, uint8_t *pt, uint32_t len, XteaStateKey_T *skey)
{
	XteaEncrypt(ct, pt, len, skey);
}


/**************************************************************************************************\
\**************************************************************************************************/
static void xtea_encrypt(const uint8_t *pt, uint8_t *ct, XteaStateKey_T *skey)
{
	uint32_t y;
	uint32_t z;
	uint32_t r;

	LOAD32L(y, &pt[0]);
	LOAD32L(z, &pt[4]);

	for (r = 0; r < 32; r += 4)
	{
		y = y + ((((z<<4)^(z>>5)) + z) ^ skey->A[r]);
		z = z + ((((y<<4)^(y>>5)) + y) ^ skey->B[r]);

		y = y + ((((z<<4)^(z>>5)) + z) ^ skey->A[r+1]);
		z = z + ((((y<<4)^(y>>5)) + y) ^ skey->B[r+1]);

		y = y + ((((z<<4)^(z>>5)) + z) ^ skey->A[r+2]);
		z = z + ((((y<<4)^(y>>5)) + y) ^ skey->B[r+2]);

		y = y + ((((z<<4)^(z>>5)) + z) ^ skey->A[r+3]);
		z = z + ((((y<<4)^(y>>5)) + y) ^ skey->B[r+3]);
	}

	STORE32L(y, &ct[0]);
	STORE32L(z, &ct[4]);
}

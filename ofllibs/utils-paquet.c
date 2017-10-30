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

#include "utils-paquet.h"
#include "string.h"
#include "stdio.h"

/*
 * Generate CRC16
 */
#define POLY 0x8408
/*
//                                      16   12   5
// this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
// This works out to be 0x1021, but the way the algorithm works
// lets us use 0x8408 (the reverse of the bit pattern).  The high
// bit is always assumed to be set, thus we only use 16 bits to
// represent the 17 bit value.
*/
unsigned short crc16(uint8_t *data_p, unsigned short length)
{
      unsigned char i;
      unsigned int data;
      unsigned int crc = 0xffff;

      if (length == 0)
            return (~crc);

      do
      {
            for (i=0, data=(unsigned int)0xff & *data_p++;
                 i < 8; 
                 i++, data >>= 1)
            {
                  if ((crc & 0x0001) ^ (data & 0x0001))
                        crc = (crc >> 1) ^ POLY;
                  else  crc >>= 1;
            }
      } while (--length);

      crc = ~crc;
      data = crc;
      crc = (crc << 8) | (data >> 8 & 0xff);

      return (crc);
}

/*
Byte 0 : 0(reserved)0(Pan compression)0(Ack)0(No Frame Pending)0(No security)001(Data)
		 00010010 => 0x01
Byte 1 : 11(SrcAddrMode)00(Frame Version)11(DstAddrMode)0(InfoElementPresent)0(Seq# suppression)
		 11001100 => 0xCC
*/ 

/* 
 * Mke packets
 */ 
uint8_t paquet2packet(paquet *pq,volatile packet_t *pk){
	uint8_t ret=1;
	memset((uint8_t*)pk->data, 0, MAX_PAYLOAD_SIZE); 
	// Generate CRC16
	unsigned short crc=crc16(pq->data,pq->datalen);
	pq->crc[0]=(crc & 0xFF); // FIXME
	pq->crc[1]=(crc << 8)& 0xFF; // FIXME

	// Build packet
	pk->length = pq->datalen +3+2+8+2+8+2; // 3(FrameControl/SeqId)2(dPan)8(dAddr)2(sPan)8(sAddra)+(X)Len+2(CRC)
	pk->offset = 0;
	pk->data[0] = 0x01;  // Frame control 1
	pk->data[1] = 0xCC;  // Frame control 2
	pk->data[2] = 0x0;   // Seq ID
	memcpy((uint8_t*) pk->data+3 , 			pq->dpan, 2); 
	memcpy((uint8_t*) pk->data+3+2,    		pq->dmac, 8); 
	memcpy((uint8_t*) pk->data+3+2+8 , 		pq->span, 2); 
	memcpy((uint8_t*) pk->data+3+2+8+2, 	pq->smac, 8); 
	memcpy((uint8_t*) pk->data+3+2+8+2+8 , 	pq->data, pq->datalen);
	memcpy((uint8_t*) pk->data+3+2+8+2+8 + pq->datalen , pq->crc, 2); 
	return(ret);
}

/* 
 * Decode packet
 */ 
uint8_t packet2paquet(volatile packet_t *pk, paquet *pq){
	uint8_t ret=1; 
	memset(pq->data, 0, PAQUET_MAX_DATASIZE); 
	if (pk->length < (3+2+8+2+8+2)){ // min size (length,offset(2), [sd]mac(8), crc(2)
		return(-1);
	}
	pq->datalen= pk->length - (3+2+8+2+8+2);
	memcpy(pq->dpan, (uint8_t*) pk->data +  3 , 2); 
	memcpy(pq->dmac, (uint8_t*) pk->data +  3+2 , 8); 
	memcpy(pq->span, (uint8_t*) pk->data +  3+2+8 , 2); 
	memcpy(pq->smac, (uint8_t*) pk->data +  3+2+8+2 , 8); 
	memcpy(pq->data, (uint8_t*) pk->data +  3+2+8+2+8 , pq->datalen);
	memcpy(pq->crc,  (uint8_t*) pk->data +  3+2+8+2+8+2  + pq->datalen, 2);
	return(ret);
}

void packet_dump(volatile packet_t *pk){
	uint8_t l=0;
	printf(" Packet Dump: "); 
	for (l=0; l< pk->length; l++) {
		printf("%02X ", pk->data[l]); 
	}
	printf("\r\n"); 



}


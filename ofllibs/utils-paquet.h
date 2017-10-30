#ifndef _UTILS_PAQUET_H
# define _UTILS_PAQUET_H


#include <mc1322x.h>
#include <board.h>


#define PAQUET_MAX_DATASIZE 120 // MAX_PAYLOAD_SIZE is 125 in packet.h




/* PAQUET Structure */

typedef struct {
        uint8_t         span[2];
        uint8_t         dpan[2];
        uint8_t         smac[8];
        uint8_t         dmac[8];
        uint8_t         datalen;
        uint8_t         data[PAQUET_MAX_DATASIZE];
        uint8_t        	crc[2]; 
} paquet;


uint8_t paquet2packet(paquet *pk, volatile packet_t *p);
uint8_t packet2paquet(volatile packet_t *p, paquet *pk);
void packet_dump(volatile packet_t *pk);

#endif

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_


#include <stdint.h>

#define BUFSZ 1024


typedef struct ringBufS{
	int16_t buf[1024];
    uint16_t head; 
    uint16_t tail; 
    uint16_t count; 
    uint16_t size; 
} ringBufS;

int ring_init(ringBufS *pRing, uint16_t ringSz) ; 
void ring_push(ringBufS *pRing, int16_t pVal); 
uint8_t ring_isempty(ringBufS *pRing); 
int16_t ring_get(ringBufS *pRing, uint16_t pPos); 
uint16_t ring_length(ringBufS *pRing); 
void ring_remove_tail(ringBufS *pRing,  uint16_t nb);
void ring_reset(ringBufS *pRing);

#endif


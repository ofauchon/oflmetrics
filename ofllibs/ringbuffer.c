#include "ringbuffer.h"
#include "string.h"
#include <stdlib.h>

#define MODULO_INC(x,y) ((x) = ((x)%(y))+1) 

/* Initialize ring for a given size
   ring->size=0 if initialization impossible 
*/
int ring_init(ringBufS *pRing, uint16_t ringSz)
{
    memset(pRing, 0, sizeof(*pRing )); 
	if (ringSz<=BUFSZ) {
		pRing->size=ringSz; 
		return 1; 
	}
	return 0; 

} 



/* Returns true if impty */
uint8_t ring_isempty(ringBufS *pRing)
{
    return (0==pRing->count);
}


void ring_reset(ringBufS *pRing)
{
	pRing->count=0; 
	pRing->head = pRing->tail; 
}



/* Returns ring size in bytes */
uint16_t ring_length(ringBufS *pRing)
{
    return (pRing->count);
}


void ring_push(ringBufS *pRing, int16_t pVal)
{
	pRing->buf[pRing->head]=pVal;
	pRing->head = MODULO_INC(pRing->head, pRing->size); 

	if (pRing->count < pRing->size)
    {
		++pRing->count;  
    } else {
		pRing->tail = MODULO_INC(pRing->tail , pRing->size); 
	}
}




int16_t ring_get(ringBufS *pRing,  uint16_t pPos)
{
	if (pPos< pRing->count){
	   return(pRing->buf[ (pRing->tail + pPos) % pRing->size]); 
    }
	//FIXME Notice error 
	return 0;
}

void ring_remove_tail(ringBufS *pRing,  uint16_t nb)
{
	if ( nb < pRing->count){
		pRing->tail = (pRing->tail + nb) % pRing->size; 
		pRing->count-=nb;
    }
}



#ifndef PRESSURE_H
# define PRESSURE_H

#define PRESSURE_POWER_PIN  28
#define PRESSURE_PIN  29

void pressure_init(uint8_t Pin);
int8_t pressure_read(uint16_t* pressure);

#endif

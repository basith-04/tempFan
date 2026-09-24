#ifndef FAN_H
#define FAN_H

#include "stm32f4xx_hal.h"

/* PA0 / TIM2 channel 1. Fan current must flow through the MOSFET stage. */
HAL_StatusTypeDef Fan_Init(void);
void Fan_SetPercent(int16_t percent);
uint8_t Fan_GetPercent(void);
void Fan_Stop(void);
void Fan_SetFullSpeed(void);

#endif

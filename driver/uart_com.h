#ifndef __UART_COM_H
#define __UART_COM_H

#include "stm32f10x.h"
#include <stdio.h>

void UART1_Init(uint32_t baudrate);
void UART1_IRQHandler_User(void);
uint8_t UART1_GetTrackingOffset(int16_t *offset_x, int16_t *offset_y);

#endif

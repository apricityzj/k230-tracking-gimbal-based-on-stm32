#ifndef __UART_COM_H
#define __UART_COM_H

#include "stm32f10x.h"
#include <stdio.h>

void UART1_Init(uint32_t baudrate);
void UART1_IRQHandler_User(void);
uint8_t UART1_GetTrackingOffset(int16_t *offset_x, int16_t *offset_y);
void UART1_SendChar(uint8_t ch);
void UART1_SendString(const char *str);
void UART1_GetRxStats(uint32_t *bytes, uint32_t *ok_frames, uint32_t *bad_frames);

#endif

#ifndef __UART_H__
#define __UART_H__

#include "stm32f10x.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART_PKT_HEADER       ((uint8_t)0x2C)
#define UART_PKT_TAIL         ((uint8_t)0x5B)
#define UART_OFFSET_MIN       ((int16_t)-1280)
#define UART_OFFSET_MAX       ((int16_t)1280)

extern volatile int16_t g_offset_x;
extern volatile int16_t g_offset_y;
extern volatile uint8_t g_offset_update_flag;

void UART_Init(uint32_t baudrate);

#ifdef __cplusplus
}
#endif

#endif

#ifndef __JOYPAD_H
#define __JOYPAD_H

#include "stm32f10x.h"

void Joypad_Init(void);
uint16_t Joypad_GetX(void);
uint16_t Joypad_GetY(void);
uint8_t Joypad_GetButton(void);

#endif

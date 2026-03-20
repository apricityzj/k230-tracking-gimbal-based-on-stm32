#ifndef __GIMBAL_H
#define __GIMBAL_H

#include "stm32f10x.h"

void Gimbal_Init(void);
void Gimbal_Update(void);
float Gimbal_GetYaw(void);
float Gimbal_GetPitch(void);

#endif

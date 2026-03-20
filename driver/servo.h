#ifndef __SERVO_H
#define __SERVO_H

#include "stm32f10x.h"

void Servo_Init(void);
void Servo_SetAngle_Yaw(float angle);
void Servo_SetAngle_Pitch(float angle);

#endif

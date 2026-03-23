#ifndef __SERVO_H__
#define __SERVO_H__

#include "stm32f10x.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SERVO_CHANNEL_X      ((uint8_t)1)
#define SERVO_CHANNEL_Y      ((uint8_t)2)

#define SERVO_PULSE_MIN_US   ((uint16_t)500)
#define SERVO_PULSE_MAX_US   ((uint16_t)2500)
#define SERVO_PULSE_MID_US   ((uint16_t)1500)

void SERVO_Init(void);
void SERVO_SetPulseUs(uint8_t channel, uint16_t pulse_us);
void SERVO_SetAngle(uint8_t channel, float angle_deg);
uint16_t SERVO_ClampPulseUs(int32_t pulse_us);

#ifdef __cplusplus
}
#endif

#endif

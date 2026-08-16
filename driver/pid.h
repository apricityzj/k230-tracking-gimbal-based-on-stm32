#ifndef __PID_H__
#define __PID_H__

#include "stm32f10x.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float kp;
    float ki;
    float kd;
    float pre_err;
    float pre_pre_err;
    float out_min;
    float out_max;
} pid_t;

void PID_Init(pid_t *pid, float kp, float ki, float kd, float out_min, float out_max);
void PID_Reset(pid_t *pid);
float PID_Calc(pid_t *pid, float setpoint, float feedback);

#ifdef __cplusplus
}
#endif

#endif

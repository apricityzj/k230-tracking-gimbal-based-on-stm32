#ifndef __PID_H__
#define __PID_H__

#include "stm32f10x.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float kp;
    float kd;
    float pre_err;
    float out_min;
    float out_max;
} pid_pd_t;

void PID_PD_Init(pid_pd_t *pid, float kp, float kd, float out_min, float out_max);
float PID_PD_Calc(pid_pd_t *pid, float setpoint, float feedback);

#ifdef __cplusplus
}
#endif

#endif

#include "pid.h"

static float PID_Limit(float value, float min_v, float max_v)
{
    if (value < min_v) { return min_v; }
    if (value > max_v) { return max_v; }
    return value;
}

/*
 * 初始化 PID 控制器
 * kp/ki/kd: 比例/积分/微分系数
 * out_min/out_max: 输出限幅（同时用于积分限幅，防止积分饱和）
 */
void PID_Init(pid_t *pid, float kp, float ki, float kd, float out_min, float out_max)
{
    if (pid == 0) { return; }
    pid->kp       = kp;
    pid->ki       = ki;
    pid->kd       = kd;
    pid->pre_err = 0.0f;
    pid->pre_pre_err = 0.0f;
    pid->out_min = out_min;
    pid->out_max = out_max;
}

/* 复位历史误差（目标切换或长时间无目标时调用） */
void PID_Reset(pid_t *pid)
{
    if (pid == 0) { return; }
    pid->pre_err = 0.0f;
    pid->pre_pre_err = 0.0f;
}

/*
 * 增量式 PID 计算（每次调用返回增量输出 Delta_u）
 * 误差  = setpoint - feedback
 * 增量  = kp*(e(k)-e(k-1)) + ki*e(k) + kd*(e(k)-2e(k-1)+e(k-2))
 */
float PID_Calc(pid_t *pid, float setpoint, float feedback)
{
    float err;
    float delta_out;

    if (pid == 0) { return 0.0f; }

    err = setpoint - feedback;
    delta_out = pid->kp * (err - pid->pre_err)
        + pid->ki * err
        + pid->kd * (err - 2.0f * pid->pre_err + pid->pre_pre_err);

    delta_out = PID_Limit(delta_out, pid->out_min, pid->out_max);

    pid->pre_pre_err = pid->pre_err;
    pid->pre_err = err;
    return delta_out;
}

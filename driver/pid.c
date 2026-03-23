#include "pid.h"

/*
 * 将输入值限制在指定区间内
 * 参数:
 *   value - 待限制的值
 *   min_v - 下限
 *   max_v - 上限
 * 返回:
 *   处于 [min_v, max_v] 区间内的值
 */
static float PID_Limit(float value, float min_v, float max_v)
{
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

/*
 * 初始化 PD 控制器参数与内部状态
 * 参数:
 *   pid     - 控制器结构体指针，不能为空
 *   kp      - 比例系数
 *   kd      - 微分系数
 *   out_min - 输出下限
 *   out_max - 输出上限
 * 返回:
 *   无
 * 说明:
 *   会将 pre_err 清零，作为首次计算的初始状态
 */
void PID_PD_Init(pid_pd_t *pid, float kp, float kd, float out_min, float out_max)
{
    if (pid == 0) {
        return;
    }

    pid->kp = kp;
    pid->kd = kd;
    pid->pre_err = 0.0f;
    pid->out_min = out_min;
    pid->out_max = out_max;
}

/*
 * 计算一次 PD 输出
 * 参数:
 *   pid      - 控制器结构体指针，不能为空
 *   setpoint - 目标值
 *   feedback - 反馈值
 * 返回:
 *   本次控制输出（已按 out_min/out_max 限幅）
 * 说明:
 *   误差定义: err = setpoint - feedback
 *   微分项: d_err = err - pre_err
 *   最终输出: kp * err + kd * d_err
 */
float PID_PD_Calc(pid_pd_t *pid, float setpoint, float feedback)
{
    float err;
    float d_err;
    float out;

    if (pid == 0) {
        return 0.0f;
    }

    err = setpoint - feedback;
    d_err = err - pid->pre_err;

    out = pid->kp * err + pid->kd * d_err;
    out = PID_Limit(out, pid->out_min, pid->out_max);

    pid->pre_err = err;
    return out;
}

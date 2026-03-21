#include "tracking_ctrl.h"
#include "servo.h"
#include "uart_com.h"

#define UART_BAUDRATE        115200
#define LOOP_MS              5

// 舵机脉宽范围（单位：us）
#define PWM_CENTER           1500.0f
#define PWM_MIN              400.0f
#define PWM_MAX              2400.0f

// 误差死区与归一化范围（单位：像素）
#define DEADZONE_PIXELS      30
#define OFFSET_FULL_SCALE    120

// 单次更新最大脉宽步长（单位：us），用于限速防抖
#define MAX_STEP_YAW_US      25.0f
#define MAX_STEP_PITCH_US    25.0f

// Pitch 一阶低通滤波系数，越小越平滑但响应更慢
#define PITCH_FILTER_ALPHA   0.10f

// 方向与轴映射开关
#define YAW_DIR              -1
#define PITCH_DIR            -1
#define SWAP_XY              0

static float pwm_yaw = PWM_CENTER;
static float pwm_pitch = PWM_CENTER;
// 滤波后的俯仰偏差，抑制上下抖动
static float pitch_filtered_offset = 0.0f;

static void Delay_ms(uint32_t ms)
{
    volatile uint32_t i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 8000; j++);
}

static void ClampAndApplyPwm(void)
{
    if (pwm_yaw > PWM_MAX) pwm_yaw = PWM_MAX;
    if (pwm_yaw < PWM_MIN) pwm_yaw = PWM_MIN;
    if (pwm_pitch > PWM_MAX) pwm_pitch = PWM_MAX;
    if (pwm_pitch < PWM_MIN) pwm_pitch = PWM_MIN;

    TIM_SetCompare1(TIM3, (uint16_t)pwm_yaw);
    TIM_SetCompare2(TIM3, (uint16_t)pwm_pitch);
}

static int16_t AbsInt16(int16_t value)
{
    return (value < 0) ? (int16_t)(-value) : value;
}

static float ComputeStepUs(int16_t offset, float max_step_us)
{
    int16_t abs_offset = AbsInt16(offset);
    float ratio;
    float step;

    if (abs_offset <= DEADZONE_PIXELS) {
        return 0.0f;
    }

    ratio = (float)(abs_offset - DEADZONE_PIXELS) / (float)(OFFSET_FULL_SCALE - DEADZONE_PIXELS);
    if (ratio > 1.0f) {
        ratio = 1.0f;
    }

    step = ratio * max_step_us;
    return (offset < 0) ? -step : step;
}



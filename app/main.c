#include "stm32f10x.h"
#include "../driver/uart.h"
#include "../driver/servo.h"
#include "../driver/pid.h"

/* 视觉偏差符号修正：若某轴画面坐标定义相反，可改为 -1.0f */
#define OFFSET_SIGN_X         (1.0f)
#define OFFSET_SIGN_Y         (1.0f)

/* 舵机机械方向修正：若舵机转向和期望相反，可改为 -1.0f */
#define SERVO_DIR_X           (1.0f)
#define SERVO_DIR_Y           (1.0f)

/* 轴到 PWM 通道映射 */
#define SERVO_CH_FOR_X        SERVO_CHANNEL_X
#define SERVO_CH_FOR_Y        SERVO_CHANNEL_Y

/* PD 参数与输出限幅，值越大响应越快但更容易振荡 */
#define PID_X_KP              (0.85f)
#define PID_X_KD              (0.26f)
#define PID_Y_KP              (0.85f)
#define PID_Y_KD              (0.26f)
#define PID_OUT_LIMIT         (15.0f)

/* 抗抖参数：像素死区与每次更新最大步进(us) */
#define OFFSET_DEADBAND       ((int16_t)25)
#define PWM_STEP_LIMIT_US     ((int32_t)18)

/* 偏差小于死区时直接置零，抑制中心附近抖动 */
static int16_t apply_deadband_i16(int16_t v, int16_t deadband)
{
    if ((v > -deadband) && (v < deadband)) {
        return 0;
    }
    return v;
}

/* 对单次控制步进做对称限幅，避免一次改动过猛 */
static int32_t clamp_step_i32(int32_t step, int32_t lim)
{
    if (step > lim) {
        return lim;
    }
    if (step < -lim) {
        return -lim;
    }
    return step;
}

int main(void)
{
    /* X/Y 两路 PD 控制器实例 */
    pid_pd_t pid_x;
    pid_pd_t pid_y;

    /* 当前舵机脉宽，初始化为中位 1500us */
    int32_t pwm_x = SERVO_PULSE_MID_US;
    int32_t pwm_y = SERVO_PULSE_MID_US;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* 初始化串口接收与 PWM 输出 */
    //UART_Init(115200U);
    UART_Init(921600U);
    SERVO_Init();

    /* 初始化 PD 参数与输出限幅 */
    PID_PD_Init(&pid_x, PID_X_KP, PID_X_KD, -PID_OUT_LIMIT, PID_OUT_LIMIT);
    PID_PD_Init(&pid_y, PID_Y_KP, PID_Y_KD, -PID_OUT_LIMIT, PID_OUT_LIMIT);

    /* 舵机上电归中 */
    SERVO_SetPulseUs(SERVO_CH_FOR_X, (uint16_t)pwm_x);
    SERVO_SetPulseUs(SERVO_CH_FOR_Y, (uint16_t)pwm_y);

    while (1) {
        if (g_offset_update_flag != 0U) {
            int16_t offset_x;
            int16_t offset_y;
            float feedback_x;
            float feedback_y;
            float out_x;
            float out_y;

            /* 原子读取中断更新的数据，防止读写竞争 */
            //关开中断处理数据
            __disable_irq();
            offset_x = g_offset_x;
            offset_y = g_offset_y;
            g_offset_update_flag = 0U;
            __enable_irq();

            /* 小偏差置零，减少目标中心附近来回抖动 */
            offset_x = apply_deadband_i16(offset_x, OFFSET_DEADBAND);
            offset_y = apply_deadband_i16(offset_y, OFFSET_DEADBAND);

            /* 坐标符号修正后作为反馈量输入 PD */
            feedback_x = OFFSET_SIGN_X * (float)offset_x;
            feedback_y = OFFSET_SIGN_Y * (float)offset_y;

            out_x = PID_PD_Calc(&pid_x, 0.0f, feedback_x);
            out_y = PID_PD_Calc(&pid_y, 0.0f, feedback_y);

            {
                /* 控制输出转为脉宽增量，并限制每帧最大步进 */
                int32_t step_x = (int32_t)(SERVO_DIR_X * out_x);
                int32_t step_y = (int32_t)(SERVO_DIR_Y * out_y);

                step_x = clamp_step_i32(step_x, PWM_STEP_LIMIT_US);
                step_y = clamp_step_i32(step_y, PWM_STEP_LIMIT_US);

                pwm_x += step_x;
                pwm_y += step_y;
            }

            /* 总脉宽二次安全限幅 */
            pwm_x = (int32_t)SERVO_ClampPulseUs(pwm_x);
            pwm_y = (int32_t)SERVO_ClampPulseUs(pwm_y);

            /* 输出到对应舵机通道 */
            SERVO_SetPulseUs(SERVO_CH_FOR_X, (uint16_t)pwm_x);
            SERVO_SetPulseUs(SERVO_CH_FOR_Y, (uint16_t)pwm_y);
        }
    }
}


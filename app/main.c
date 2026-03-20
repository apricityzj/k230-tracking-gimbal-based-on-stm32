#include "stm32f10x.h"
#include "servo.h"
#include "uart_com.h"
#include <stdio.h>

#define PWM_MIN             500.0f
#define PWM_MAX             2500.0f
#define PWM_CENTER          1500.0f
#define KP_DIV              30.0f
#define LOOP_MS             10
#define STEP_MAX_PER_LOOP   30.0f

static float pwm_yaw = PWM_CENTER;
static float pwm_pitch = PWM_CENTER;

// 简单的毫秒延时函数
void Delay_ms(uint32_t ms)
{
    // 粗略延时，假设系统时钟约为 72MHz
    volatile uint32_t i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 8000; j++); // 在 72MHz 下约等于 1ms
}

int main(void)
{
    int16_t track_x = 0;
    int16_t track_y = 0;

    // 1. 初始化舵机 PWM 输出
    Servo_Init();
    TIM_SetCompare1(TIM3, (uint16_t)PWM_CENTER);
    TIM_SetCompare2(TIM3, (uint16_t)PWM_CENTER);

    // 2. 初始化 UART1 接收视觉偏移量（波特率：115200）
    UART1_Init(115200);

    // 3. 主循环
    while(1)
    {
        // 收到完整帧后执行一次控制更新
        if (UART1_GetTrackingOffset(&track_x, &track_y)) {
            float delta_yaw = (float)track_x / KP_DIV;
            float delta_pitch = (float)track_y / KP_DIV;

            if (delta_yaw > STEP_MAX_PER_LOOP) delta_yaw = STEP_MAX_PER_LOOP;
            if (delta_yaw < -STEP_MAX_PER_LOOP) delta_yaw = -STEP_MAX_PER_LOOP;
            if (delta_pitch > STEP_MAX_PER_LOOP) delta_pitch = STEP_MAX_PER_LOOP;
            if (delta_pitch < -STEP_MAX_PER_LOOP) delta_pitch = -STEP_MAX_PER_LOOP;

            pwm_yaw += delta_yaw;
            pwm_pitch += delta_pitch;

            if (pwm_yaw > PWM_MAX) pwm_yaw = PWM_MAX;
            if (pwm_yaw < PWM_MIN) pwm_yaw = PWM_MIN;
            if (pwm_pitch > PWM_MAX) pwm_pitch = PWM_MAX;
            if (pwm_pitch < PWM_MIN) pwm_pitch = PWM_MIN;

            TIM_SetCompare1(TIM3, (uint16_t)pwm_yaw);
            TIM_SetCompare2(TIM3, (uint16_t)pwm_pitch);

            printf("X%d Y%d PWM1=%.1f PWM2=%.1f\n", track_x, track_y, pwm_yaw, pwm_pitch);
        }

        Delay_ms(LOOP_MS);
    }
}

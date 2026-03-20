#include "servo.h"

void Servo_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    // 使能时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    // PA6（TIM3_CH1）和 PA7（TIM3_CH2）配置为复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 50Hz PWM 的时基配置
    // 假设系统核心时钟为 72MHz
    // 72MHz / 72 = 1MHz 计数频率（1 tick = 1us）
    // 20000 tick = 20ms = 50Hz
    TIM_TimeBaseStructure.TIM_Period = 20000 - 1; 
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1; 
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    // 输出比较（PWM）配置
    // 脉宽 = 1500us -> 1.5ms（对应 90 度中心位）
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 1500;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    // CH1 配置（Yaw，PA6）
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

    // CH2 配置（Pitch，PA7）
    TIM_OCInitStructure.TIM_Pulse = 1500;
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);

    // 使能定时器
    TIM_Cmd(TIM3, ENABLE);
}

// 将 0 - 180 度转换为 Yaw 脉宽
void Servo_SetAngle_Yaw(float angle)
{
    // 限制角度范围
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;
    
    // 将 0-180 映射到 500-2500 脉宽（0.5ms 到 2.5ms）
    uint16_t pwm_val = (uint16_t)(500 + (angle / 180.0f) * 2000);
    TIM_SetCompare1(TIM3, pwm_val);
}

// 将 0 - 180 度转换为 Pitch 脉宽
void Servo_SetAngle_Pitch(float angle)
{
    // 限制角度范围
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;
    
    // 将 0-180 映射到 500-2500 脉宽（0.5ms 到 2.5ms）
    uint16_t pwm_val = (uint16_t)(500 + (angle / 180.0f) * 2000);
    TIM_SetCompare2(TIM3, pwm_val);
}

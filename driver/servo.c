#include "servo.h"

uint16_t SERVO_ClampPulseUs(int32_t pulse_us)
{
    if (pulse_us < SERVO_PULSE_MIN_US) {
        return SERVO_PULSE_MIN_US;
    }
    if (pulse_us > SERVO_PULSE_MAX_US) {
        return SERVO_PULSE_MAX_US;
    }
    return (uint16_t)pulse_us;
}

void SERVO_Init(void)
{
    GPIO_InitTypeDef gpio_init;
    TIM_TimeBaseInitTypeDef tim_base_init;
    TIM_OCInitTypeDef tim_oc_init;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    gpio_init.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init);

    tim_base_init.TIM_Period = 20000U - 1U;
    tim_base_init.TIM_Prescaler = 72U - 1U;
    tim_base_init.TIM_ClockDivision = TIM_CKD_DIV1;
    tim_base_init.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &tim_base_init);

    tim_oc_init.TIM_OCMode = TIM_OCMode_PWM1;
    tim_oc_init.TIM_OutputState = TIM_OutputState_Enable;
    tim_oc_init.TIM_Pulse = SERVO_PULSE_MID_US;
    tim_oc_init.TIM_OCPolarity = TIM_OCPolarity_High;

    TIM_OC1Init(TIM3, &tim_oc_init);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

    TIM_OC2Init(TIM3, &tim_oc_init);
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}

void SERVO_SetPulseUs(uint8_t channel, uint16_t pulse_us)
{
    uint16_t pulse = SERVO_ClampPulseUs((int32_t)pulse_us);

    if (channel == SERVO_CHANNEL_X) {
        TIM_SetCompare1(TIM3, pulse);
    } else if (channel == SERVO_CHANNEL_Y) {
        TIM_SetCompare2(TIM3, pulse);
    }
}

void SERVO_SetAngle(uint8_t channel, float angle_deg)
{
    float pulse;

    if (angle_deg < 0.0f) {
        angle_deg = 0.0f;
    }
    if (angle_deg > 180.0f) {
        angle_deg = 180.0f;
    }

    pulse = 500.0f + (angle_deg / 180.0f) * 2000.0f;
    SERVO_SetPulseUs(channel, (uint16_t)(pulse + 0.5f));
}

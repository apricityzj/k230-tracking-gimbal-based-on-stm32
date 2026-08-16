#include "stm32f10x.h"
#include "../driver/servo.h"

/*
 * Servo Physical Limit Test
 * 
 * Both servos sweep from min to max pulse width
 * Observe actual rotation range on each axis
 * 
 * Expected pulse range: 500-2500us (2000us span)
 * Fixed spec: X max 270°, Y max 180°
 */

void delay_ms(uint32_t ms)
{
    uint32_t i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 8; j++);
}

int main(void)
{
    uint16_t pulse;
    const uint16_t SERVO_MIN = 500;
    const uint16_t SERVO_MAX = 2500;
    const uint16_t STEP = 50;  /* 50us step */
    const uint16_t STEP_DELAY = 100;  /* 100ms per step */

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SERVO_Init();

    /* Loop forever: sweep min->max->min */
    while (1) {
        /* Sweep from 500us to 2500us (min to max) */
        for (pulse = SERVO_MIN; pulse <= SERVO_MAX; pulse += STEP) {
            SERVO_SetPulseUs(SERVO_CHANNEL_X, pulse);
            SERVO_SetPulseUs(SERVO_CHANNEL_Y, pulse);
            delay_ms(STEP_DELAY);
        }

        delay_ms(500);  /* Pause at max */

        /* Sweep from 2500us back to 500us */
        for (pulse = SERVO_MAX; pulse >= SERVO_MIN; pulse -= STEP) {
            SERVO_SetPulseUs(SERVO_CHANNEL_X, pulse);
            SERVO_SetPulseUs(SERVO_CHANNEL_Y, pulse);
            delay_ms(STEP_DELAY);
        }

        delay_ms(500);  /* Pause at min */

        /* Return to fixed center angle */
        SERVO_SetAngle(SERVO_CHANNEL_X, SERVO_X_CENTER_ANGLE_DEG);
        SERVO_SetAngle(SERVO_CHANNEL_Y, SERVO_Y_CENTER_ANGLE_DEG);
        delay_ms(1000);
    }
}

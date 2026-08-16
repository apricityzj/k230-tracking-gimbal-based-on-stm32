#include "stm32f10x.h"
#include "../driver/uart.h"
#include "../driver/servo.h"
#include <string.h>
#include <stdlib.h>

/*
 * K230 云台跟踪系统 - STM32F103 控制端
 *
 * 功能说明：
 * - 通过 UART 接收 K230 发送的 X/Y 偏移（0xAA...0xBB 数据包）
 * - 使用 PID 控制 X/Y 两轴舵机
 * - 通过 TIM3 CH1/CH2（PA6/PA7）输出 PWM 驱动舵机
 * - USART2 (PA2) 输出调试数据，VOFA+ FireWater 协议
 *
 * 串口协议：115200 波特率，6 字节包
 * [0xAA, XH, XL, YH, YL, 0xBB]
 * X/Y 为 int16 补码
 *
 * 调试输出：PA2 -> USB转串口 -> VOFA+ (921600 bps, FireWater)
 * 格式: offset_x,offset_y,angle_x,angle_y\n
 */

//#define X_KP                  (0.013f)
//#define X_KI                  (0.0001f)
//#define X_KD                  (0.035f)
//#define Y_KP                  (0.013f)
//#define Y_KI                  (0.0001f)
//#define Y_KD                  (0.035f)

//#define DEADZONE_Y            (4.0f)
//#define DEADZONE_X            (4.0f)
//#define INTEGRAL_LIMIT        (1000.0f)

#define X_KP                  (0.013f)
#define X_KI                  (0.0001f)
#define X_KD                  (0.035f)
#define Y_KP                  (0.015f)
#define Y_KI                  (0.0001f)
#define Y_KD                  (0.013f)

#define DEADZONE_Y            (4.0f)
#define DEADZONE_X            (4.0f)
#define INTEGRAL_LIMIT        (1000.0f)

/* ── USART2 调试串口 (PA2 TX) ── */
static void USART2_Debug_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    gpio.GPIO_Pin   = GPIO_Pin_2;          /* PA2 = TX */
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    USART_StructInit(&usart);
    usart.USART_BaudRate            = baudrate;
    usart.USART_WordLength          = USART_WordLength_8b;
    usart.USART_StopBits            = USART_StopBits_1;
    usart.USART_Parity              = USART_Parity_No;
    usart.USART_Mode                = USART_Mode_Tx;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART2, &usart);
    USART_Cmd(USART2, ENABLE);
}

/* ── USART2 字符串发送（不依赖 printf，避免 semihosting 卡死）─── */
static void dbg_send_str(const char *s)
{
    while (*s) {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, (uint8_t)(*s++));
    }
}

/* itoa 简易实现（Keil C 标准库可能不带） */
static void dbg_send_int(int32_t val)
{
    char buf[12];
    int i = 0;
    uint32_t u;
    if (val < 0) {
        dbg_send_str("-");
        u = (uint32_t)(-val);
    } else {
        u = (uint32_t)val;
    }
    do {
        buf[i++] = '0' + (u % 10);
        u /= 10;
    } while (u && i < 11);
    buf[i] = '\0';
    /* reverse */
    {
        int j;
        for (j = 0; j < i / 2; j++) {
            char t = buf[j]; buf[j] = buf[i-1-j]; buf[i-1-j] = t;
        }
    }
    dbg_send_str(buf);
}

static void dbg_send_csv(int32_t a, int32_t b, int32_t c, int32_t d,
                          int32_t e, int32_t f, int32_t g, int32_t h)
{
    int32_t vals[8] = {a, b, c, d, e, f, g, h};
    int k;
    for (k = 0; k < 7; k++) {
        dbg_send_int(vals[k]); dbg_send_str(",");
    }
    dbg_send_int(vals[7]); dbg_send_str("\r\n");
}

static float limit_f(float v, float min_v, float max_v)
{
    if (v < min_v) {
        return min_v;
    }
    if (v > max_v) {
        return max_v;
    }
    return v;
}

static float servo_pid_step(float current, float target,
                            float kp, float ki, float kd,
                            float deadzone,
                            float *integral, float *err_last)
{
    float err = current - target;
    float derivative;

    if ((err > -deadzone) && (err < deadzone)) {
        *integral *= 0.9f;
        *err_last = 0.0f;
        return 0.0f;
    }

    *integral += err * 0.3f;
    *integral = limit_f(*integral, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);

    derivative = err - *err_last;
    *err_last = err;

    return kp * err + ki * (*integral) + kd * derivative;
}

int main(void)
{
    int16_t offset_x;
    int16_t offset_y;
    float angle_x;
    float angle_y;
    float x_integral;
    float y_integral;
    float x_err_last;
    float y_err_last;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* 初始化 UART1（与 K230 通信）和舵机 PWM */
    UART_Init(115200U);
    SERVO_Init();
    USART2_Debug_Init(115200U);   /* PA2 -> USB转串口 -> VOFA+ */

    /* 启动测试帧：确认 USART2 输出正常 */
    dbg_send_str("STM32 USART2 OK\r\n");

    /* 按固定规格初始化到中位：X=135°(270°一半)，Y=90°(180°一半) */
    angle_x = SERVO_X_CENTER_ANGLE_DEG;
    angle_y = SERVO_Y_CENTER_ANGLE_DEG;
    x_integral = 0.0f;
    y_integral = 0.0f;
    x_err_last = 0.0f;
    y_err_last = 0.0f;
    SERVO_SetAngle(SERVO_CHANNEL_X, angle_x);
    SERVO_SetAngle(SERVO_CHANNEL_Y, angle_y);

    while (1) {
        /* 等待串口收到新数据包 */
        if (g_offset_update_flag == 0U) {
            continue;
        }

        /* 从中断更新的全局变量中安全读取偏移 */
        __disable_irq();
        offset_x = g_offset_x;
        offset_y = g_offset_y;
        g_offset_update_flag = 0U;
        __enable_irq();

        /* 位置式 PID 输出作为角度增量，累加到当前角度 */
        angle_x += (-1.0f) * servo_pid_step((float)offset_x, 0.0f,
                                  X_KP, X_KI, X_KD,
                                  DEADZONE_X,
                                  &x_integral, &x_err_last);
        angle_y += (-1.0f) * servo_pid_step((float)offset_y, 0.0f,
                                  Y_KP, Y_KI, Y_KD,
                                  DEADZONE_Y,
                                  &y_integral, &y_err_last);

        /* 按轴规格限幅 */
        angle_x = limit_f(angle_x, 0.0f, SERVO_X_MAX_ANGLE_DEG);
        angle_y = limit_f(angle_y, 0.0f, SERVO_Y_MAX_ANGLE_DEG);

        /* 更新舵机角度 */
        SERVO_SetAngle(SERVO_CHANNEL_X, angle_x);
        SERVO_SetAngle(SERVO_CHANNEL_Y, angle_y);

        /* 调试输出 8ch：SV_X, PV_X, SV_Y, PV_Y, Ix, AngleX, Iy, AngleY */
        dbg_send_csv(
            0, (int32_t)offset_x,                   /* SV_X=0, PV_X=偏差 */
            0, (int32_t)offset_y,                   /* SV_Y=0, PV_Y=偏差 */
            (int32_t)(x_integral * 10),              /* X积分×10 */
            (int32_t)(angle_x * 10),                /* X角度×10 */
            (int32_t)(y_integral * 10),              /* Y积分×10 */
            (int32_t)(angle_y * 10));               /* Y角度×10 */
    }
}


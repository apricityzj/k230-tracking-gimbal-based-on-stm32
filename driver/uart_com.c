#include "uart_com.h"
#include <string.h>

#define UART_RING_BUF_LEN 256

// 串口接收环形缓冲区：中断写入，主循环读取
static volatile uint8_t rx_ring_buf[UART_RING_BUF_LEN];
static volatile uint16_t rx_ring_head = 0;
static volatile uint16_t rx_ring_tail = 0;

// 接收统计：总字节数、成功帧数、坏帧数
static volatile uint32_t rx_byte_count = 0;
static volatile uint32_t rx_ok_frame_count = 0;
static volatile uint32_t rx_bad_frame_count = 0;

static uint8_t UART1_RingPopByte(uint8_t *out)
{
    uint16_t tail;

    if (out == 0) {
        return 0;
    }

    // 与中断写指针共享，临界区内弹出一个字节
    __disable_irq();
    if (rx_ring_head == rx_ring_tail) {
        __enable_irq();
        return 0;
    }

    tail = rx_ring_tail;
    *out = rx_ring_buf[tail];
    rx_ring_tail = (uint16_t)((tail + 1) % UART_RING_BUF_LEN);
    __enable_irq();
    return 1;
}

static uint8_t UART1_IsDigit(uint8_t ch)
{
    return (ch >= '0' && ch <= '9') ? 1 : 0;
}

static int16_t UART1_ApplySign(int32_t value, int8_t sign)
{
    int32_t signed_value = (sign < 0) ? -value : value;

    // 限制到 int16_t 可表示范围，避免异常输入溢出
    if (signed_value > 32767) {
        signed_value = 32767;
    }
    if (signed_value < -32768) {
        signed_value = -32768;
    }

    return (int16_t)signed_value;
}

static void UART1_ResetParser(uint8_t *state, int8_t *x_sign, int8_t *y_sign, int32_t *x_abs, int32_t *y_abs, uint8_t *x_has_digit, uint8_t *y_has_digit)
{
    *state = 0;
    *x_sign = 1;
    *y_sign = 1;
    *x_abs = 0;
    *y_abs = 0;
    *x_has_digit = 0;
    *y_has_digit = 0;
}

void USART1_IRQHandler(void)
{
    UART1_IRQHandler_User();
}

void UART1_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    // PA9: USART1_TX, PA10: USART1_RX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 仅开启接收非空中断，由中断搬运数据到环形缓冲区
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
}

void UART1_IRQHandler_User(void)
{
    uint8_t data;
    uint16_t next_head;

    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        // 读取 DR 会清 RXNE；将字节写入环形缓冲区
        data = (uint8_t)USART_ReceiveData(USART1);
        rx_byte_count++;

        next_head = (uint16_t)((rx_ring_head + 1) % UART_RING_BUF_LEN);
        if (next_head != rx_ring_tail) {
            rx_ring_buf[rx_ring_head] = data;
            rx_ring_head = next_head;
        }

        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }

    if (USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET) {
        // 过载错误按手册顺序读 SR/DR 清标志
        volatile uint16_t tmp;
        tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
    }
}

uint8_t UART1_GetTrackingOffset(int16_t *offset_x, int16_t *offset_y)
{
    // 状态机协议：X[+|-]dddY[+|-]ddd，支持大小写 x/y
    // state=0 等待 X；state=1 解析 X；state=2 解析 Y
    static uint8_t state = 0;
    static int8_t x_sign = 1;
    static int8_t y_sign = 1;
    static int32_t x_abs = 0;
    static int32_t y_abs = 0;
    static uint8_t x_has_digit = 0;
    static uint8_t y_has_digit = 0;
    uint8_t ch;

    if (offset_x == 0 || offset_y == 0) {
        return 0;
    }

    while (UART1_RingPopByte(&ch)) {
        if (ch == 'X' || ch == 'x') {
            // 若上一帧 Y 已完整，先提交上一帧，再以新 X 开始下一帧
            if (state == 2 && y_has_digit) {
                *offset_x = UART1_ApplySign(x_abs, x_sign);
                *offset_y = UART1_ApplySign(y_abs, y_sign);
                rx_ok_frame_count++;
                UART1_ResetParser(&state, &x_sign, &y_sign, &x_abs, &y_abs, &x_has_digit, &y_has_digit);
                state = 1;
                return 1;
            }

            UART1_ResetParser(&state, &x_sign, &y_sign, &x_abs, &y_abs, &x_has_digit, &y_has_digit);
            state = 1;
            continue;
        }

        if (state == 0) {
            continue;
        }

        if (state == 1) {
            if ((ch == '+' || ch == '-') && x_has_digit == 0 && x_abs == 0) {
                x_sign = (ch == '-') ? -1 : 1;
                continue;
            }

            if (UART1_IsDigit(ch)) {
                x_abs = x_abs * 10 + (int32_t)(ch - '0');
                x_has_digit = 1;
                continue;
            }

            if ((ch == 'Y' || ch == 'y') && x_has_digit) {
                state = 2;
                continue;
            }

            // X 段格式错误，丢弃当前帧
            rx_bad_frame_count++;
            UART1_ResetParser(&state, &x_sign, &y_sign, &x_abs, &y_abs, &x_has_digit, &y_has_digit);
            continue;
        }

        if ((ch == '+' || ch == '-') && y_has_digit == 0 && y_abs == 0) {
            y_sign = (ch == '-') ? -1 : 1;
            continue;
        }

        if (UART1_IsDigit(ch)) {
            y_abs = y_abs * 10 + (int32_t)(ch - '0');
            y_has_digit = 1;
            continue;
        }

        if (y_has_digit) {
            // Y 段已经有数字，遇到分隔/终止字符即可提交整帧
            *offset_x = UART1_ApplySign(x_abs, x_sign);
            *offset_y = UART1_ApplySign(y_abs, y_sign);
            rx_ok_frame_count++;
            UART1_ResetParser(&state, &x_sign, &y_sign, &x_abs, &y_abs, &x_has_digit, &y_has_digit);
            return 1;
        }

        // Y 段还没有有效数字就异常，记为坏帧
        rx_bad_frame_count++;
        UART1_ResetParser(&state, &x_sign, &y_sign, &x_abs, &y_abs, &x_has_digit, &y_has_digit);
    }

    if (state == 2 && y_has_digit) {
        // 缓冲区读空但当前帧已完整，也可以直接提交
        *offset_x = UART1_ApplySign(x_abs, x_sign);
        *offset_y = UART1_ApplySign(y_abs, y_sign);
        rx_ok_frame_count++;
        UART1_ResetParser(&state, &x_sign, &y_sign, &x_abs, &y_abs, &x_has_digit, &y_has_digit);
        return 1;
    }

    return 0;
}

void UART1_GetRxStats(uint32_t *bytes, uint32_t *ok_frames, uint32_t *bad_frames)
{
    if (bytes) {
        *bytes = rx_byte_count;
    }
    if (ok_frames) {
        *ok_frames = rx_ok_frame_count;
    }
    if (bad_frames) {
        *bad_frames = rx_bad_frame_count;
    }
}

void UART1_SendChar(uint8_t ch)
{
    // 轮询等待发送数据寄存器空
    while ((USART1->SR & USART_SR_TXE) == 0) {
    }
    USART_SendData(USART1, ch);
}

void UART1_SendString(const char *str)
{
    if (str == 0) {
        return;
    }

    while (*str) {
        UART1_SendChar((uint8_t)(*str));
        str++;
    }
}

int fputc(int ch, FILE *f)
{
    while ((USART1->SR & 0X40) == 0);
    USART_SendData(USART1, (uint8_t)ch);
    return ch;
}

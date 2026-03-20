#include "uart_com.h"
#include <string.h>

#define UART_RX_BUF_LEN 32

static volatile uint8_t rx_state = 0;
static volatile uint8_t rx_idx = 0;
static volatile char rx_work_buf[UART_RX_BUF_LEN];
static volatile char rx_frame_buf[UART_RX_BUF_LEN];
static volatile uint8_t rx_frame_ready = 0;

void UART1_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    // USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // USART1_RX   PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // USART1 配置
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

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
}

void UART1_IRQHandler_User(void)
{
    uint8_t res;

    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        res = (uint8_t)USART_ReceiveData(USART1);

        if (res == 'X') {
            rx_state = 1;
            rx_idx = 0;
            rx_work_buf[rx_idx++] = (char)res;
        } else if (rx_state == 1) {
            if (res == '\n' || res == '\r') {
                if (rx_idx > 0) {
                    rx_work_buf[rx_idx] = '\0';

                    if (rx_frame_ready == 0) {
                        memcpy((void *)rx_frame_buf, (const void *)rx_work_buf, rx_idx + 1);
                        rx_frame_ready = 1;
                    }
                }
                rx_state = 0;
            } else {
                if (rx_idx < (UART_RX_BUF_LEN - 1)) {
                    rx_work_buf[rx_idx++] = (char)res;
                } else {
                    rx_state = 0;
                    rx_idx = 0;
                }
            }
        }

        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

uint8_t UART1_GetTrackingOffset(int16_t *offset_x, int16_t *offset_y)
{
    char local_buf[UART_RX_BUF_LEN];
    int x = 0;
    int y = 0;

    if (offset_x == 0 || offset_y == 0) {
        return 0;
    }

    if (rx_frame_ready == 0) {
        return 0;
    }

    __disable_irq();
    memcpy(local_buf, (const void *)rx_frame_buf, UART_RX_BUF_LEN);
    rx_frame_ready = 0;
    __enable_irq();

    if (sscanf(local_buf, "X%dY%d", &x, &y) == 2) {
        *offset_x = (int16_t)x;
        *offset_y = (int16_t)y;
        return 1;
    }

    return 0;
}

// 将 printf 重定向到 USART1
int fputc(int ch, FILE *f)
{
    // 等待发送数据寄存器为空
    while((USART1->SR & 0X40) == 0);
    // 发送字符
    USART_SendData(USART1, (uint8_t)ch);
    return ch;
}

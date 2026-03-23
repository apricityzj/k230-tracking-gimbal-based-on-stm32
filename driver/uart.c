#include "uart.h"

typedef enum {
    UART_RX_WAIT_HEADER = 0,
    UART_RX_GET_DATA,
    UART_RX_WAIT_TAIL
} uart_rx_state_t;

volatile int16_t g_offset_x = 0;
volatile int16_t g_offset_y = 0;
volatile uint8_t g_offset_update_flag = 0;

static volatile uart_rx_state_t s_rx_state = UART_RX_WAIT_HEADER;
static volatile uint8_t s_data_buf[4];
static volatile uint8_t s_data_idx = 0;

/*
 * 按字节解析串口协议帧：包头(1B) + 数据(4B: x高,x低,y高,y低) + 包尾(1B)
 * 参数 data: 当前接收到的1字节
 * 行为: 解析成功后更新 g_offset_x/g_offset_y，并置位 g_offset_update_flag
 */
static void UART_ParseByte(uint8_t data)
{
    int16_t x;
    int16_t y;

    switch (s_rx_state) {
    case UART_RX_WAIT_HEADER:
        if (data == UART_PKT_HEADER) {
            s_data_idx = 0;
            s_rx_state = UART_RX_GET_DATA;
        }
        break;

    case UART_RX_GET_DATA:
        s_data_buf[s_data_idx++] = data;
        if (s_data_idx >= 4U) {
            s_rx_state = UART_RX_WAIT_TAIL;
        }
        break;

    case UART_RX_WAIT_TAIL:
        if (data == UART_PKT_TAIL) {
            x = (int16_t)(((uint16_t)s_data_buf[0] << 8) | s_data_buf[1]);
            y = (int16_t)(((uint16_t)s_data_buf[2] << 8) | s_data_buf[3]);

            if ((x >= UART_OFFSET_MIN) && (x <= UART_OFFSET_MAX) &&
                (y >= UART_OFFSET_MIN) && (y <= UART_OFFSET_MAX)) {
                g_offset_x = x;
                g_offset_y = y;
                g_offset_update_flag = 1U;
            }
        }

        s_rx_state = UART_RX_WAIT_HEADER;
        s_data_idx = 0;
        break;

    default:
        s_rx_state = UART_RX_WAIT_HEADER;
        s_data_idx = 0;
        break;
    }
}

/*
 * 初始化 USART1 串口与接收中断
 * 参数 baudrate: 串口波特率（如 115200）
 * 行为: 配置 GPIOA(PA9/PA10)、USART1 参数、RXNE 中断与 NVIC
 */
void UART_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio_init;
    USART_InitTypeDef usart_init;
    NVIC_InitTypeDef nvic_init;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);

    gpio_init.GPIO_Pin = GPIO_Pin_9;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init);

    gpio_init.GPIO_Pin = GPIO_Pin_10;
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio_init);

    usart_init.USART_BaudRate = baudrate;
    usart_init.USART_WordLength = USART_WordLength_8b;
    usart_init.USART_StopBits = USART_StopBits_1;
    usart_init.USART_Parity = USART_Parity_No;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &usart_init);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    nvic_init.NVIC_IRQChannel = USART1_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_init.NVIC_IRQChannelSubPriority = 1;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    USART_Cmd(USART1, ENABLE);
}

/*
 * USART1 中断服务函数（ISR）
 * 行为: RXNE 置位时读取1字节并交给状态机解析
 */
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t rx = (uint8_t)USART_ReceiveData(USART1);
        UART_ParseByte(rx);
    }
}

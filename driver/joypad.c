#include "joypad.h"

void Joypad_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    // 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1 | RCC_APB2Periph_AFIO, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6); // 72MHz / 6 = 12MHz ADC 时钟

    // PA0（VRx）和 PA1（VRy）配置为模拟输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // PA2（SW）配置为上拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // ADC1 配置
    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    // 使能 ADC1
    ADC_Cmd(ADC1, ENABLE);

    // 校准
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}

// 读取指定 ADC 通道的辅助函数
static uint16_t Joypad_ReadADC(uint8_t ADC_Channel)
{
    // 配置规则通道
    ADC_RegularChannelConfig(ADC1, ADC_Channel, 1, ADC_SampleTime_55Cycles5);
    
    // 启动转换
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    
    // 等待转换结束
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    
    return ADC_GetConversionValue(ADC1);
}

// 读取 X 轴（PA0 -> 通道 0）
uint16_t Joypad_GetX(void)
{
    return Joypad_ReadADC(ADC_Channel_0);
}

// 读取 Y 轴（PA1 -> 通道 1）
uint16_t Joypad_GetY(void)
{
    return Joypad_ReadADC(ADC_Channel_1);
}

// 读取按键状态（PA2，低电平有效）
// 按下返回 1，否则返回 0
uint8_t Joypad_GetButton(void)
{
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_2) == Bit_RESET) {
        return 1;
    }
    return 0;
}

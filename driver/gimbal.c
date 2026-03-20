#include "gimbal.h"
#include "joypad.h"
#include "servo.h"

// 常量定义
#define GIMBAL_CENTER_ANGLE     90.0f
#define JOYSTICK_CENTER         2048
#define DEADBAND                400   // 死区，用于防止回中抖动：2048 +/- 400
#define MAX_SPEED               3.0f  // 每次更新周期的最大转动角度

// 静态状态变量
static float current_yaw = GIMBAL_CENTER_ANGLE;
static float current_pitch = GIMBAL_CENTER_ANGLE;

void Gimbal_Init(void)
{
    // 初始化硬件模块
    Joypad_Init();
    Servo_Init();
    
    // 将舵机设置到中心位置
    Servo_SetAngle_Yaw(current_yaw);
    Servo_SetAngle_Pitch(current_pitch);
}

void Gimbal_Update(void)
{
    // 1. 自动回中功能
    if (Joypad_GetButton()) {
        current_yaw = GIMBAL_CENTER_ANGLE;
        current_pitch = GIMBAL_CENTER_ANGLE;
    } 
    // 2. 比例控制
    else {
        uint16_t x_val = Joypad_GetX();
        uint16_t y_val = Joypad_GetY();
        
        // 根据 X 轴偏移计算 Yaw 的比例速度
        if (x_val < (JOYSTICK_CENTER - DEADBAND)) {
            float ratio = (JOYSTICK_CENTER - DEADBAND - x_val) / (float)(JOYSTICK_CENTER - DEADBAND);
            current_yaw += ratio * MAX_SPEED;
        } else if (x_val > (JOYSTICK_CENTER + DEADBAND)) {
            float ratio = (x_val - (JOYSTICK_CENTER + DEADBAND)) / (float)(4095 - (JOYSTICK_CENTER + DEADBAND));
            current_yaw -= ratio * MAX_SPEED;
        }
        
        // 根据 Y 轴偏移计算 Pitch 的比例速度
        if (y_val < (JOYSTICK_CENTER - DEADBAND)) {
            float ratio = (JOYSTICK_CENTER - DEADBAND - y_val) / (float)(JOYSTICK_CENTER - DEADBAND);
            current_pitch += ratio * MAX_SPEED;
        } else if (y_val > (JOYSTICK_CENTER + DEADBAND)) {
            float ratio = (y_val - (JOYSTICK_CENTER + DEADBAND)) / (float)(4095 - (JOYSTICK_CENTER + DEADBAND));
            current_pitch -= ratio * MAX_SPEED;
        }
    }
    
    // 3. 角度限幅
    if (current_yaw < 0.0f) current_yaw = 0.0f;
    if (current_yaw > 180.0f) current_yaw = 180.0f;
    
    if (current_pitch < 0.0f) current_pitch = 0.0f;
    if (current_pitch > 180.0f) current_pitch = 180.0f;
    
    // 4. 更新舵机输出
    Servo_SetAngle_Yaw(current_yaw);
    Servo_SetAngle_Pitch(current_pitch);
}

// 提供给外部日志/监控使用的获取接口
float Gimbal_GetYaw(void) {
    return current_yaw;
}

float Gimbal_GetPitch(void) {
    return current_pitch;
}

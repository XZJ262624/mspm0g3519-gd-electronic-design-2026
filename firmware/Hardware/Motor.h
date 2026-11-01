/**
  ****************************************************************************
  * @file    Motor.h
  * @author  XZJxsdss
  * @date    2025_7_30
  * @brief   电机测速及控制
  ****************************************************************************
**/
#ifndef __MOTOR_H
#define __MOTOR_H

#include <stdbool.h>
#include <stdint.h>

void MotorControl_SetStartRampEnabled(bool enabled);

//初始化
void MotorControl_Init(void);

// TimerG0每10ms调用
void MotorControl_Update10ms(void);

// 上层设置目标速度
void MotorControl_SetLeftTarget(
    int32_t count_per_10ms);
void MotorControl_SetRightTarget(
    int32_t count_per_10ms);
void MotorControl_SetLeftRightTarget(int32_t count_per_10ms_L, int32_t count_per_10ms_R);
void MotorControl_GetTargets(
    int32_t *left_count_per_10ms,
    int32_t *right_count_per_10ms);

void MotorControl_SetEnabled(bool enabled);
void MotorControl_ToggleEnabled(void);
bool MotorControl_IsEnabled(void);

// 读取编码器进行PID计算
void MotorDriver_SetLeftCommand(int16_t command);
void MotorDriver_SetRightCommand(int16_t command);

// void PIDControlTest(void);

#endif

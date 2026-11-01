/**
  ****************************************************************************
  * @file    pwm.h
  * @author  XZJxsdss
  * @date    2025_7_30
  * @brief   PWM输出
  ****************************************************************************
**/
#include <stdint.h>
#ifndef PWM_H
#define PWM_H

void PWM_Init(void);

void PWM_SetLeftIN1Duty(uint16_t duty);
void PWM_SetLeftIN2Duty(uint16_t duty);

void PWM_SetRightIN1Duty(uint16_t duty);
void PWM_SetRightIN2Duty(uint16_t duty);

// void SetPWM_TG12_CH0(uint16_t Duty);
// void SetPWM_TG12_CH1(uint16_t Duty);
// void SetPWM_TG6_CH0(uint16_t Duty);
// void SetPWM_TG6_CH1(uint16_t Duty);

#endif

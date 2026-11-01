/**
  ****************************************************************************
  * @file    pwm.c
  * @author  XZJxsdss
  * @date    2025_7_30
  * @brief   PWM输出
  ****************************************************************************
**/
#include "Board_Config.h"
#include "PWM.h"

#define PWM_DUTY_MAX 100U

static uint16_t PWM_ClampDuty(uint16_t duty)
{
    if (duty > PWM_DUTY_MAX)
    {
        return PWM_DUTY_MAX;
    }

    return duty;
}

static uint32_t PWM_DutyToCompare(
    uint16_t duty)
{
    uint32_t period;

    duty = PWM_ClampDuty(duty);

    period =
        DL_TimerA_getLoadValue(
            BSP_MOTOR_PWM_INST);

    return
        ((period * (uint32_t)duty) + 50U) /
        100U;
}

void PWM_Init(void)
{
    DL_TimerA_startCounter(
        BSP_MOTOR_PWM_INST);
}

void PWM_SetLeftIN1Duty(uint16_t duty)
{
    DL_TimerA_setCaptureCompareValue(
        BSP_MOTOR_PWM_INST,
        PWM_DutyToCompare(duty),
        BSP_MOTOR_LEFT_IN1_PWM_CC_INDEX);
}

void PWM_SetLeftIN2Duty(uint16_t duty)
{
    DL_TimerA_setCaptureCompareValue(
        BSP_MOTOR_PWM_INST,
        PWM_DutyToCompare(duty),
        BSP_MOTOR_LEFT_IN2_PWM_CC_INDEX);
}

void PWM_SetRightIN1Duty(uint16_t duty)
{
    DL_TimerA_setCaptureCompareValue(
        BSP_MOTOR_PWM_INST,
        PWM_DutyToCompare(duty),
        BSP_MOTOR_RIGHT_IN1_PWM_CC_INDEX);
}

void PWM_SetRightIN2Duty(uint16_t duty)
{
    DL_TimerA_setCaptureCompareValue(
        BSP_MOTOR_PWM_INST,
        PWM_DutyToCompare(duty),
        BSP_MOTOR_RIGHT_IN2_PWM_CC_INDEX);
}
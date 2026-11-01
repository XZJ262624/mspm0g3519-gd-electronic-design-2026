/**
  ****************************************************************************
  * @file    Timer.c
  * @author  XZJxsdss
  * @date    2025_8_4
  * @brief   仪式感函数
  ****************************************************************************
**/
#include "Board_Config.h"
#include "Timer.h"

// static void PWM_SetSafeUpdateMethod(void)
// {
//     DL_TimerG_setCaptCompUpdateMethod(
//         BSP_MOTOR_RIGHT_PWM_INST,
//         DL_TIMER_CC_UPDATE_METHOD_ZERO_EVT,
//         DL_TIMERG_CAPTURE_COMPARE_0_INDEX);

//     DL_TimerG_setCaptCompUpdateMethod(
//         BSP_MOTOR_RIGHT_PWM_INST,
//         DL_TIMER_CC_UPDATE_METHOD_ZERO_EVT,
//         DL_TIMERG_CAPTURE_COMPARE_1_INDEX);

//     DL_TimerG_setCaptCompUpdateMethod(
//         BSP_MOTOR_LEFT_PWM_INST,
//         DL_TIMER_CC_UPDATE_METHOD_ZERO_EVT,
//         DL_TIMERG_CAPTURE_COMPARE_0_INDEX);

//     DL_TimerG_setCaptCompUpdateMethod(
//         BSP_MOTOR_LEFT_PWM_INST,
//         DL_TIMER_CC_UPDATE_METHOD_ZERO_EVT,
//         DL_TIMERG_CAPTURE_COMPARE_1_INDEX);
// }

// void Timer_Init(void)
// {
//     PWM_SetSafeUpdateMethod();

//     TimerG6_Init();//电机PWM
//     TimerG12_Init();//电机PWM

//     TimerG7_Init();//负责：按键消抖,20ms
//     TimerG0_Init();//负责：电机速度环pid,10ms一次中断
// }

void Timer_Init(void)
{
    DL_TimerG_startCounter(
        BSP_KEY_TIMER_INST);

    DL_TimerG_startCounter(
        BSP_SPEED_TIMER_INST);
}

void TimerG0_Init(void)
{
    DL_TimerG_startCounter(TIMER_G0_INST);
}

void TimerG6_Init(void)
{
    // DL_TimerG_startCounter(BSP_MOTOR_RIGHT_PWM_INST);
}

void TimerG7_Init(void)
{
    DL_TimerG_startCounter(TIMER_G7_INST);
}

void TimerG12_Init(void)
{
    // DL_TimerG_startCounter(BSP_MOTOR_LEFT_PWM_INST);
}

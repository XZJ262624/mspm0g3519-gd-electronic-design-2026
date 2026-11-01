/**
  ****************************************************************************
  * @file    Clock.h
  * @author  移植自bilibili_Torris_Yin
  * @date    2025_8_7
  * @brief   为M0G3507配置系统内置的嘀嗒定时器
  * @version unknown
  ****************************************************************************
**/

/**
  ****************************************************************************
  * @date    2026_7_20
  * @brief   去掉裸全局变量
  ****************************************************************************
**/
#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

void SysTick_Init(void);
void Clock_Tick1ms(void);
uint32_t Clock_GetMs(void);
void mspm0_delay_ms(uint32_t delay_ms);

#endif
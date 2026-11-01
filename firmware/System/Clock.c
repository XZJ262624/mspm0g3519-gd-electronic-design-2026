/**
  ****************************************************************************
  * @file    Clock.c
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

#include "ti_msp_dl_config.h"
#include "Clock.h"

static volatile uint32_t s_tick_ms;

void Clock_Tick1ms(void)
{
    s_tick_ms++;
}

uint32_t Clock_GetMs(void)
{
    return s_tick_ms;
}

void mspm0_delay_ms(uint32_t delay_ms)
{
    uint32_t start_time;

    start_time = Clock_GetMs();

    while ((uint32_t)(
        Clock_GetMs() - start_time) < delay_ms)
    {
    }
}

//
//当前GPIO编码器方案下：
//GPIO编码器优先级0
//TimerG0优先级1
//SysTick优先级2
//TimerG7优先级3
//
void SysTick_Init(void)
{
    DL_SYSTICK_config(CPUCLK_FREQ / 1000U);

    NVIC_SetPriority(SysTick_IRQn, 2U);
}
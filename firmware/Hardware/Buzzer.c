#include "ti_msp_dl_config.h"
#include "Board_Config.h"
#include "Buzzer.h"

/* 按键提示音持续时间，可修改 */
#define BUZZER_BEEP_DURATION_MS    (80U)

static volatile uint8_t s_buzzer_state = 0U;
static volatile uint32_t s_buzzer_remaining_ms = 0U;


/* 初始化蜂鸣器 */
void Buzzer_Init(void)
{
    s_buzzer_remaining_ms = 0U;
    Buzzer_Off();
}


/* 打开蜂鸣器 */
void Buzzer_On(void)
{
    DL_GPIO_setPins(
        BSP_BUZZER_PORT,
        BSP_BUZZER_PIN);

    s_buzzer_state = 1U;
}


/* 关闭蜂鸣器 */
void Buzzer_Off(void)
{
    DL_GPIO_clearPins(
        BSP_BUZZER_PORT,
        BSP_BUZZER_PIN);

    s_buzzer_state = 0U;
    s_buzzer_remaining_ms = 0U;
}


/* 非阻塞短响一次 */
void Buzzer_Beep(void)
{
    /*
     * 再次按键时重新开始计时，
     * 不会因为原来正在响而关闭蜂鸣器。
     */
    s_buzzer_remaining_ms =
        BUZZER_BEEP_DURATION_MS;

    Buzzer_On();
}


/* 由1ms SysTick中断调用 */
void Buzzer_Update1ms(void)
{
    if (s_buzzer_remaining_ms > 0U)
    {
        s_buzzer_remaining_ms--;

        if (s_buzzer_remaining_ms == 0U)
        {
            Buzzer_Off();
        }
    }
}


uint8_t Buzzer_GetState(void)
{
    return s_buzzer_state;
}
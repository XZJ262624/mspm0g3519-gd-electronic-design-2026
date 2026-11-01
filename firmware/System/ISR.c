#include "Board_Config.h"
#include "ISR.h"
#include "Clock.h"
#include "Encoder.h"
#include "Key.h"
#include "Motor.h"
#include "UART.h"
#include "Buzzer.h"

#define TASK_NUMBER_MIN    (1U)
#define TASK_NUMBER_MAX    (14U)

volatile uint8_t g_current_task = TASK_NUMBER_MIN;
volatile bool g_task_confirmed = false;


/* SW3、SW4、SW5：切换到下一个任务 */
static void Task_SwitchNext(void)
{
    /*
     * 必须先取消确认，再修改任务号，
     * 防止新任务在按SW2确认前被发送。
     */
    g_task_confirmed = false;

    g_current_task++;

    if (g_current_task > TASK_NUMBER_MAX)
    {
        g_current_task = TASK_NUMBER_MIN;
    }

    Buzzer_Beep();
}


/* SW2：确认当前任务 */
static void Task_Confirm(void)
{
    g_task_confirmed = true;

    Buzzer_Beep();
}


/* 按键中断任务入口 */
static void Key_InterruptTask(KeyEvent_t key_event)
{
    switch (key_event)
    {
        case KEY_EVENT_SW2_PRESS:
            Task_Confirm();
            break;

        case KEY_EVENT_SW3_PRESS:
            Task_SwitchNext();
            break;

        case KEY_EVENT_SW4_PRESS:
            Task_SwitchNext();
            break;

        case KEY_EVENT_SW5_PRESS:
            Task_SwitchNext();
            break;

        case KEY_EVENT_NONE:
        default:
            break;
    }
}

void BSP_UART1_HANDLER(void)
{
    UART_IRQHandler(BSP_WIRELESS_UART_PORT);
}

void BSP_UART7_HANDLER(void)
{
    UART_IRQHandler(BSP_GYRO_UART_PORT);
}

void ISR_Init(void)
{
    NVIC_ClearPendingIRQ(BSP_SPEED_TIMER_IRQN);
    NVIC_ClearPendingIRQ(BSP_KEY_TIMER_IRQN);

    NVIC_EnableIRQ(BSP_SPEED_TIMER_IRQN);
    NVIC_EnableIRQ(BSP_KEY_TIMER_IRQN);
}

void SysTick_Handler(void)
{
    Clock_Tick1ms();
    Buzzer_Update1ms();
}

void BSP_SPEED_TIMER_HANDLER(void)
{
    switch (DL_TimerG_getPendingInterrupt(
        BSP_SPEED_TIMER_INST))
    {
        case DL_TIMER_IIDX_ZERO:
            MotorControl_Update10ms();
            break;

        default:
            break;
    }
}

void BSP_KEY_TIMER_HANDLER(void)
{
    KeyEvent_t event;

    switch (DL_TimerG_getPendingInterrupt(
        BSP_KEY_TIMER_INST))
    {
        case DL_TIMER_IIDX_ZERO:
            event = Key_Update20ms();
            if (event != KEY_EVENT_NONE)
            {
                Key_InterruptTask(event);
            }

            break;

        default:
            break;
    }
}

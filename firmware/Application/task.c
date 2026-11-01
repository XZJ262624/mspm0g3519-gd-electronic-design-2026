#include "task.h"

#include "Board_Config.h"
#include "UART.h"

#define TASK_FRAME_SIZE    (7U)
#define TASK_NUMBER_MIN    (1U)
#define TASK_NUMBER_MAX    (14U)

static uint8_t s_task_frame[TASK_FRAME_SIZE] =
{
    0xB2,
    0x2B,
    0x00,   /* task */
    0x00,   /* left high */
    0x00,   /* left low */
    0x00,   /* right high */
    0x00    /* right low */
};

/* 当前帧已经写入UART4 TX FIFO的字节数 */
static uint8_t s_task_tx_index;

bool Task_Init(void)
{
    Task_ResetSend();

    return UART_AttachConfigured(
        BSP_TASK_UART_PORT);
}

void Task_ResetSend(void)
{
    s_task_tx_index = 0U;
}

bool Task_Send(
    uint8_t current_task,
    int16_t left_speed,
    int16_t right_speed)
{
    uint16_t left_raw;
    uint16_t right_raw;

    if ((current_task < TASK_NUMBER_MIN) ||
        (current_task > TASK_NUMBER_MAX))
    {
        Task_ResetSend();
        return false;
    }

    /*
     * 只在一帧开始发送时更新数据。
     * 避免UART FIFO满时一帧内部的数据发生变化。
     */
    if (s_task_tx_index == 0U)
    {
        left_raw = (uint16_t)left_speed;
        right_raw = (uint16_t)right_speed;

        s_task_frame[2] = current_task;

        s_task_frame[3] =
            (uint8_t)(left_raw >> 8);

        s_task_frame[4] =
            (uint8_t)(left_raw & 0xFFU);

        s_task_frame[5] =
            (uint8_t)(right_raw >> 8);

        s_task_frame[6] =
            (uint8_t)(right_raw & 0xFFU);
    }

    while (s_task_tx_index < TASK_FRAME_SIZE)
    {
        if (!UART_TryWriteByte(
                BSP_TASK_UART_PORT,
                s_task_frame[s_task_tx_index]))
        {
            return false;
        }

        s_task_tx_index++;
    }

    Task_ResetSend();
    return true;
}
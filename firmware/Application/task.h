#ifndef TASK_H
#define TASK_H

#include <stdbool.h>
#include <stdint.h>

bool Task_Init(void);

bool Task_Send(
    uint8_t current_task,
    int16_t left_speed,
    int16_t right_speed);

/* 清除尚未完成的发送进度 */
void Task_ResetSend(void);

#endif
#ifndef ISR_H
#define ISR_H

#include <stdbool.h>
#include <stdint.h>

extern volatile uint8_t g_current_task;
extern volatile bool g_task_confirmed;

void ISR_Init(void);

#endif
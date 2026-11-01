#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

void Buzzer_Init(void);

void Buzzer_On(void);

void Buzzer_Off(void);

void Buzzer_Beep(void);

void Buzzer_Update1ms(void);

uint8_t Buzzer_GetState(void);

#endif
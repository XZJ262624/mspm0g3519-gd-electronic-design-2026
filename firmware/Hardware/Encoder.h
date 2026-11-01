/**
  ****************************************************************************
  * @file    Encoder.h
  * @author  XZJxsdss
  * @date    2025_8_2
  * @brief   霍尔编码器电机测速头文件
  ****************************************************************************
**/
#ifndef __ENCODER_H
#define __ENCODER_H
#include <stdint.h>

void Encoder_Init(void);

typedef struct
{
    int32_t left_count;
    int32_t right_count;
} EncoderSample_t;

void Encoder_ReadDelta(EncoderSample_t *sample);
void Encoder_GetLatestSample(
    EncoderSample_t *sample);
const volatile EncoderSample_t *
Encoder_GetLatestSamplePtr(void);

#endif


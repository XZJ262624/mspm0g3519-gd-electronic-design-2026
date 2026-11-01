/**
  ****************************************************************************
  * @file    Encoder.c
  * @author  XZJxsdss
  * @date    2025_7_21
  * @brief   霍尔编码器电机测速
  * @version V 2.0
  * 更新于2025_8_1, 移植至M0G3507
  ****************************************************************************
**/
/**
  ****************************************************************************
  * @date    2025_8_12
  * @brief   四驱方案：由“//注释分组一”开始取消注释，至注释部分下方最近有效代码行
  * @date    2026_7_22
  * @brief   xsdss重构
  * @date    2025_7_23
  * @brief   改为硬件QEI
  ****************************************************************************
**/

#include "Board_Config.h"
#include "Encoder.h"

static uint16_t s_left_previous_count  = 0U;//左后电机当前控制周期内的有符号编码器计数
static uint16_t s_right_previous_count = 0U;//右后电机当前控制周期内的有符号编码器计数
//采样结果
volatile EncoderSample_t g_encoder_latest_sample =
{
    0,0
};

void Encoder_Init(void)
{
    //QEI计数器从0开始
    DL_TimerG_setTimerCount(
        BSP_QEI_LEFT_INST,
        0U);

    DL_TimerG_setTimerCount(
        BSP_QEI_RIGHT_INST,
        0U);

    //启动两个计数器
    DL_TimerG_startCounter(
        BSP_QEI_LEFT_INST);

    DL_TimerG_startCounter(
        BSP_QEI_RIGHT_INST);

    s_left_previous_count =
        (uint16_t)DL_TimerG_getTimerCount(
            BSP_QEI_LEFT_INST);

    s_right_previous_count =
        (uint16_t)DL_TimerG_getTimerCount(
            BSP_QEI_RIGHT_INST);

    g_encoder_latest_sample.left_count  = 0;
    g_encoder_latest_sample.right_count = 0;
}

void Encoder_ReadDelta(EncoderSample_t *sample)
{
    uint16_t left_now;
    uint16_t right_now;

    int32_t left_delta;
    int32_t right_delta;

    /*
     * 读取两个QEI硬件计数器。
     */
    left_now =
        (uint16_t)DL_TimerG_getTimerCount(
            BSP_QEI_LEFT_INST);

    right_now =
        (uint16_t)DL_TimerG_getTimerCount(
            BSP_QEI_RIGHT_INST);

    left_delta =
        (int32_t)(int16_t)(
            left_now -
            s_left_previous_count);

    right_delta =
        (int32_t)(int16_t)(
            right_now -
            s_right_previous_count);

    //保存本次计数，供下一周期计算差值
    s_left_previous_count  = left_now;
    s_right_previous_count = right_now;

    //统一车辆前进方向为正数
    left_delta *= BSP_QEI_LEFT_SIGN;
    right_delta *= BSP_QEI_RIGHT_SIGN;

    g_encoder_latest_sample.left_count =
        left_delta;

    g_encoder_latest_sample.right_count =
        right_delta;

    if (sample != 0)
    {
        sample->left_count =
            left_delta;

        sample->right_count =
            right_delta;
    }
}

void Encoder_GetLatestSample(
    EncoderSample_t *sample)
{
    uint32_t previous_primask;

    if (sample == 0)
    {
        return;
    }

    previous_primask = __get_PRIMASK();
    __disable_irq();

    sample->left_count =
        g_encoder_latest_sample.left_count;

    sample->right_count =
        g_encoder_latest_sample.right_count;

    if ((previous_primask & 1U) == 0U)
    {
        __enable_irq();
    }
}

const volatile EncoderSample_t *
Encoder_GetLatestSamplePtr(void)
{
    return &g_encoder_latest_sample;
}

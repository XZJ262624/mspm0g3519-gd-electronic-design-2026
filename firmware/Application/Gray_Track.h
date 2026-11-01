#ifndef __GRAY_TRACK_H__
#define __GRAY_TRACK_H__

#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"

#define LQ_GRAY_CHANNEL_COUNT       (16U)
#define LQ_GRAY_TRACK_PERIOD_MS     (10U)

/* ADC原始值，范围0～4095 */
extern volatile uint16_t
    g_lq_gray_adc[LQ_GRAY_CHANNEL_COUNT];

/*
 * 归一化黑线强度：
 * 白底 = 0
 * 黑线 = 1000
 */
extern volatile uint16_t
    g_lq_gray_strength[LQ_GRAY_CHANNEL_COUNT];

/*
 * 加权计算后的黑线位置：
 * 负数 = 黑线在左侧
 * 0    = 黑线在中央
 * 正数 = 黑线在右侧
 */
extern volatile int32_t
    g_lq_gray_position;

/*
 * 巡线PID转向输出：
 * 正数 = 左转
 * 负数 = 右转
 */
extern volatile int32_t
    g_lq_gray_steering;

/* 16路归一化强度总和 */
extern volatile uint32_t
    g_lq_gray_strength_sum;

/* 黑白标定数据是否有效 */
extern volatile bool
    g_lq_gray_calibration_valid;

/* 当前是否检测到黑线 */
extern volatile bool
    g_lq_gray_line_detected;

/* 中央D7～D10是否同时检测到黑线 */
extern volatile bool
    g_lq_gray_center_four_black;

void Gray_Track_Init(void);

/* 停车或重新选择任务时复位巡线控制 */
void Gray_Track_ResetControl(void);

/*
 * main中可以连续调用。
 * 内部每10ms才进行一次ADC扫描和巡线PID计算。
 */
void track(uint8_t current_task);

#endif

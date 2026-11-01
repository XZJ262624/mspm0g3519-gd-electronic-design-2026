#include <stdbool.h>
#include <stdint.h>

#include "Board_Config.h"
#include "Clock.h"
#include "Gray_Track.h"
#include "Motor.h"
#include "PID.h"


/*
 * ==================== 标定与调参区域 ====================
 */
static const uint16_t
s_lq_gray_white_adc[LQ_GRAY_CHANNEL_COUNT] = {
    892U, 868U, 702U, 710U,
    614U, 727U, 635U, 667U,
    569U, 576U, 592U, 611U,
    628U, 598U, 701U, 824U
};

static const uint16_t
s_lq_gray_black_adc[LQ_GRAY_CHANNEL_COUNT] = {
    4095U, 4095U, 4095U, 4095U,
    4095U, 4095U, 4095U, 4095U,
    4095U, 4095U, 4095U, 4095U,
    4095U, 4095U, 4095U, 4095U
};

//巡线位置PID参数。
#define LINE_PID_KP                        (0.080f)
#define LINE_PID_KI                        (0.003f)
#define LINE_PID_KD                        (0.002f)
#define LINE_PID_D_FILTER_S                (0.020f)

#define LINE_BASE_SPEED                    (800)/* 左右轮最大差速修正量 */
#define LINE_MAX_CORRECTION                (800)
#define LINE_TARGET_MIN                    (50)/* 巡线目标速度限制 */
#define LINE_TARGET_MAX                    (1600)

//task2巡线速度
#define LINE_BASE_SPEED_TASK2              (1000)
//task4、task5、task6共用巡线速度
#define LINE_BASE_SPEED_TASK456            (680)


/* 每路丢弃第1次采样，然后对2次有效采样求平均 */
#define LQ_GRAY_VALID_SAMPLES              (2U)

/* 某一路黑白ADC差值小于此值，认为没有正确标定 */
#define LQ_GRAY_MIN_CALIBRATION_SPAN       (100U)

/* 归一化后的最大黑线强度 */
#define LQ_GRAY_NORMALIZED_MAX             (1000U)

/* 小于该值的弱信号当作白底噪声 */
#define LQ_GRAY_STRENGTH_NOISE_FLOOR       (30U)

/*
 * 第一次标定时保持为0，小车停车但继续采集ADC。
 * 填写黑白标定值后改成1。
 */
#define LQ_GRAY_TRACK_RUN_ENABLE           (1U)

/*
 * 各路灰度输入开关：
 * 1U = 正常使用
 * 0U = 屏蔽，始终视为没检测到黑线
 */
#define LQ_GRAY_D1_ENABLE    (1U)
#define LQ_GRAY_D2_ENABLE    (1U)
#define LQ_GRAY_D3_ENABLE    (1U)
#define LQ_GRAY_D4_ENABLE    (1U)
#define LQ_GRAY_D5_ENABLE    (1U)
#define LQ_GRAY_D6_ENABLE    (1U)
#define LQ_GRAY_D7_ENABLE    (1U)
#define LQ_GRAY_D8_ENABLE    (1U)
#define LQ_GRAY_D9_ENABLE    (1U)
#define LQ_GRAY_D10_ENABLE   (1U)
#define LQ_GRAY_D11_ENABLE   (1U)
#define LQ_GRAY_D12_ENABLE   (1U)
#define LQ_GRAY_D13_ENABLE   (1U)
#define LQ_GRAY_D14_ENABLE   (1U)
#define LQ_GRAY_D15_ENABLE   (1U)
#define LQ_GRAY_D16_ENABLE   (1U)

/*
 * 16路归一化强度总和小于该值时认为丢线。
 */
#define LINE_DETECT_MIN_SUM                (250U)

/*
 * 中央4路D7～D10的黑线停车阈值。
 * 归一化强度：白底=0，黑线=1000。
 */
#define LINE_STOP_BLACK_THRESHOLD    (700U)

#define LINE_STOP_D6_INDEX           (5U)
#define LINE_STOP_D11_INDEX          (10U)
#define LINE_STOP_REQUIRED_COUNT     (4U)

#define LINE_CONTROL_PERIOD_S \
    ((float)LQ_GRAY_TRACK_PERIOD_MS / 1000.0f)


/*
 * MUX通道0～15依次写入D1～D16标定槽位。
 *
 * 如果模块丝印编号与MUX实际通道完全相反，
 * 应在采集标定数据之前改成：
 * {
 *     15U, 14U, 13U, 12U, 11U, 10U, 9U, 8U,
 *      7U,  6U,  5U,  4U,  3U,  2U, 1U, 0U
 * }
 */
static const uint8_t
s_lq_gray_channel_map[LQ_GRAY_CHANNEL_COUNT] = {
     0U,  1U,  2U,  3U,
     4U,  5U,  6U,  7U,
     8U,  9U, 10U, 11U,
    12U, 13U, 14U, 15U
};

static const uint8_t
s_lq_gray_channel_enabled[LQ_GRAY_CHANNEL_COUNT] = {
    LQ_GRAY_D1_ENABLE,
    LQ_GRAY_D2_ENABLE,
    LQ_GRAY_D3_ENABLE,
    LQ_GRAY_D4_ENABLE,
    LQ_GRAY_D5_ENABLE,
    LQ_GRAY_D6_ENABLE,
    LQ_GRAY_D7_ENABLE,
    LQ_GRAY_D8_ENABLE,
    LQ_GRAY_D9_ENABLE,
    LQ_GRAY_D10_ENABLE,
    LQ_GRAY_D11_ENABLE,
    LQ_GRAY_D12_ENABLE,
    LQ_GRAY_D13_ENABLE,
    LQ_GRAY_D14_ENABLE,
    LQ_GRAY_D15_ENABLE,
    LQ_GRAY_D16_ENABLE
};

/*
 * D1～D16的位置权重。
 *
 * 符号方向延续当前P15工程，权重间隔保持1000。
 * 左右符号错误时应整体取反本数组，不要在标定后改通道映射。
 */
static const int16_t
s_lq_gray_position_weight[LQ_GRAY_CHANNEL_COUNT] = {
     -7500,  -6500,  -5500,   -4500,
     -3500,  -2500,  -1500,   -500,
     500,    1500,   2500,   3500,
     4500,  5500,  6500,   7500
};


volatile uint16_t
g_lq_gray_adc[LQ_GRAY_CHANNEL_COUNT] = {
    0U, 0U, 0U, 0U,
    0U, 0U, 0U, 0U,
    0U, 0U, 0U, 0U,
    0U, 0U, 0U, 0U
};


volatile uint16_t
g_lq_gray_strength[LQ_GRAY_CHANNEL_COUNT] = {
    0U, 0U, 0U, 0U,
    0U, 0U, 0U, 0U,
    0U, 0U, 0U, 0U,
    0U, 0U, 0U, 0U
};


volatile int32_t
g_lq_gray_position = 0;

volatile int32_t
g_lq_gray_steering = 0;

volatile uint32_t
g_lq_gray_strength_sum = 0U;

volatile bool
g_lq_gray_calibration_valid = false;

volatile bool
g_lq_gray_line_detected = false;

volatile bool
g_lq_gray_center_four_black = false;


/*
 * 保留你原来的变量。
 * fre现在表示连续丢线周期数。
 */
uint8_t judgement = 0U;
uint16_t fre = 0U;


static PID_Controller_t s_line_pid;

static uint32_t
s_last_control_ms = 0U;

/*
 * 巡线PID算出的最终期望速度。
 * main循环持续把它送入原来的速度斜坡函数。
 */
static int32_t
s_left_target_command = 0;

static int32_t
s_right_target_command = 0;

static bool LQ_Gray_CenterFourAreBlack(void)
{
    uint8_t sensor_index;
    uint8_t black_count = 0U;
    /*
     * 检查D6～D11。
     * 其中至少4路检测到黑线时满足停车条件。
     */
    for (sensor_index = LINE_STOP_D6_INDEX;
         sensor_index <= LINE_STOP_D11_INDEX;
         sensor_index++)
    {
        if (g_lq_gray_strength[sensor_index] >=
            LINE_STOP_BLACK_THRESHOLD)
        {
            black_count++;

            if (black_count >= LINE_STOP_REQUIRED_COUNT)
            {
                return true;
            }
        }
    }

    return false;
}


/*
 * int32范围限制。
 */
static int32_t LQ_Gray_ClampInt32(
    int32_t value,
    int32_t minimum,
    int32_t maximum)
{
    if (value > maximum)
    {
        return maximum;
    }

    if (value < minimum)
    {
        return minimum;
    }

    return value;
}


/*
 * 浮点数四舍五入为int32。
 */
static int32_t LQ_Gray_FloatToInt32(
    float value)
{
    if (value >= 0.0f)
    {
        return (int32_t)(value + 0.5f);
    }

    return (int32_t)(value - 0.5f);
}


/*
 * 设置16选1模拟多路复用器地址。
 *
 * S0～S3只是GPIO地址输出，不是4路ADC输入：
 * S0为最低位，S3为最高位。每次只选中一个通道，
 * 选通后的模拟量统一从AO/PB25这一条ADC输入读取。
 */
static void LQ_Gray_SelectChannel(
    uint8_t channel)
{
    uint32_t set_pins = 0U;

    DL_GPIO_clearPins(
        BSP_LQ_GRAY_MUX_PORT,
        BSP_LQ_GRAY_S0_PIN |
        BSP_LQ_GRAY_S1_PIN |
        BSP_LQ_GRAY_S2_PIN |
        BSP_LQ_GRAY_S3_PIN);

    if ((channel & 0x01U) != 0U)
    {
        set_pins |= BSP_LQ_GRAY_S0_PIN;
    }

    if ((channel & 0x02U) != 0U)
    {
        set_pins |= BSP_LQ_GRAY_S1_PIN;
    }

    if ((channel & 0x04U) != 0U)
    {
        set_pins |= BSP_LQ_GRAY_S2_PIN;
    }

    if ((channel & 0x08U) != 0U)
    {
        set_pins |= BSP_LQ_GRAY_S3_PIN;
    }

    if (set_pins != 0U)
    {
        DL_GPIO_setPins(
            BSP_LQ_GRAY_MUX_PORT,
            set_pins);
    }
}


/*
 * 执行一次ADC转换。
 */
static uint16_t LQ_Gray_ReadADCOnce(void)
{
    uint16_t result;

    DL_ADC12_clearInterruptStatus(
        BSP_LQ_GRAY_ADC_INST,
        BSP_LQ_GRAY_ADC_RESULT_INTERRUPT);

    DL_ADC12_enableConversions(
        BSP_LQ_GRAY_ADC_INST);

    DL_ADC12_startConversion(
        BSP_LQ_GRAY_ADC_INST);

    while ((DL_ADC12_getRawInterruptStatus(
                BSP_LQ_GRAY_ADC_INST,
                BSP_LQ_GRAY_ADC_RESULT_INTERRUPT) &
            BSP_LQ_GRAY_ADC_RESULT_INTERRUPT) == 0U)
    {
    }

    result = DL_ADC12_getMemResult(
        BSP_LQ_GRAY_ADC_INST,
        BSP_LQ_GRAY_ADC_MEM);

    DL_ADC12_clearInterruptStatus(
        BSP_LQ_GRAY_ADC_INST,
        BSP_LQ_GRAY_ADC_RESULT_INTERRUPT);

    return result;
}


/*
 * 读取一个灰度通道。
 */
static uint16_t LQ_Gray_ReadChannel(
    uint8_t channel)
{
    uint32_t sum = 0U;
    uint8_t sample_index;

    LQ_Gray_SelectChannel(channel);

    /*
     * 丢弃切换模拟通道后的第一次采样。
     */
    (void)LQ_Gray_ReadADCOnce();

    for (sample_index = 0U;
         sample_index < LQ_GRAY_VALID_SAMPLES;
         sample_index++)
    {
        sum += LQ_Gray_ReadADCOnce();
    }

    return (uint16_t)(
        sum / LQ_GRAY_VALID_SAMPLES);
}


/*
 * 单ADC轮询扫描D1～D16：
 * 选择一路 -> 丢弃切换后的首个样本 -> 平均有效样本
 * -> 保存结果 -> 再切换到下一路。
 */
static void LQ_Gray_Scan(void)
{
    uint8_t sensor_index;

    for (sensor_index = 0U;
         sensor_index < LQ_GRAY_CHANNEL_COUNT;
         sensor_index++)
    {
        /*
         * 正常巡线时跳过屏蔽通道。
         *
         * 标定模式下即使通道被屏蔽也仍扫描，
         * 便于重新判断该路是否可以恢复使用。
         */
        if ((LQ_GRAY_TRACK_RUN_ENABLE != 0U) &&
            (s_lq_gray_channel_enabled[
                sensor_index] == 0U))
        {
            g_lq_gray_adc[sensor_index] = 0U;
            g_lq_gray_strength[sensor_index] = 0U;
            continue;
        }

        g_lq_gray_adc[sensor_index] =
            LQ_Gray_ReadChannel(
                s_lq_gray_channel_map[
                    sensor_index]);
    }
}


/*
 * 将每路ADC独立归一化。
 *
 * 公式：
 *
 * strength =
 *     (ADC - 白底ADC) * 1000
 *     / (黑线ADC - 白底ADC)
 *
 * 程序自动兼容：
 * 黑低白高
 * 黑高白低
 *
 * 最终统一为：
 * 白底 = 0
 * 黑线 = 1000
 */
static bool LQ_Gray_UpdateNormalized(void)
{
    bool calibration_valid = true;
    uint8_t sensor_index;

    for (sensor_index = 0U;
         sensor_index < LQ_GRAY_CHANNEL_COUNT;
         sensor_index++)
    {
        int32_t adc_value;
        int32_t black_value;
        int32_t white_value;
        int32_t span;
        int32_t absolute_span;
        int32_t normalized;

        if (s_lq_gray_channel_enabled[
                sensor_index] == 0U)
        {
            /*
            * 屏蔽通道默认没有检测到黑线，
            * 同时不会造成整体标定无效。
            */
            g_lq_gray_strength[
                sensor_index] = 0U;

            continue;
        }

        adc_value =
            (int32_t)g_lq_gray_adc[
                sensor_index];

        black_value =
            (int32_t)s_lq_gray_black_adc[
                sensor_index];

        white_value =
            (int32_t)s_lq_gray_white_adc[
                sensor_index];

        span = black_value - white_value;

        absolute_span =
            (span >= 0) ? span : -span;

        /*
         * 黑白差太小，认为该路没有正确标定。
         */
        if (absolute_span <
            (int32_t)LQ_GRAY_MIN_CALIBRATION_SPAN)
        {
            g_lq_gray_strength[
                sensor_index] = 0U;

            calibration_valid = false;
            continue;
        }

        normalized =
            ((adc_value - white_value) *
             (int32_t)LQ_GRAY_NORMALIZED_MAX) /
            span;

        normalized = LQ_Gray_ClampInt32(
            normalized,
            0,
            (int32_t)LQ_GRAY_NORMALIZED_MAX);

        /*
         * 去除白底附近的小幅噪声。
         */
        if (normalized <
            (int32_t)LQ_GRAY_STRENGTH_NOISE_FLOOR)
        {
            normalized = 0;
        }

        g_lq_gray_strength[
            sensor_index] =
            (uint16_t)normalized;
    }

    g_lq_gray_calibration_valid =
        calibration_valid;

    return calibration_valid;
}


/*
 * 根据16路归一化强度计算黑线位置。
 */
static bool LQ_Gray_CalculatePosition(
    int32_t *position)
{
    int32_t weighted_sum = 0;
    uint32_t strength_sum = 0U;
    uint8_t sensor_index;

    for (sensor_index = 0U;
         sensor_index < LQ_GRAY_CHANNEL_COUNT;
         sensor_index++)
    {
        if (s_lq_gray_channel_enabled[
                sensor_index] == 0U)
        {
            continue;
        }
        strength_sum +=
            (uint32_t)g_lq_gray_strength[
                sensor_index];

        weighted_sum +=
            (int32_t)g_lq_gray_strength[
                sensor_index] *
            (int32_t)s_lq_gray_position_weight[
                sensor_index];
    }

    g_lq_gray_strength_sum =
        strength_sum;

    /*
     * 归一化总强度太小，认为16路都没有看到黑线。
     */
    if (strength_sum <
        LINE_DETECT_MIN_SUM)
    {
        return false;
    }

    if (position != 0)
    {
        *position =
            weighted_sum /
            (int32_t)strength_sum;
    }

    return true;
}


void Gray_Track_Init(void)
{
    DL_GPIO_clearPins(
        BSP_LQ_GRAY_MUX_PORT,
        BSP_LQ_GRAY_S0_PIN |
        BSP_LQ_GRAY_S1_PIN |
        BSP_LQ_GRAY_S2_PIN |
        BSP_LQ_GRAY_S3_PIN);

    /*
     * 初始化巡线位置PID。
     */
    PID_Init(
        &s_line_pid,
        LINE_PID_KP,
        LINE_PID_KI,
        LINE_PID_KD,
        LINE_CONTROL_PERIOD_S,
        LINE_PID_D_FILTER_S,
        -(float)LINE_MAX_CORRECTION,
        (float)LINE_MAX_CORRECTION,
        -(float)LINE_MAX_CORRECTION,
        (float)LINE_MAX_CORRECTION);

    /*
     * 上电先读取一次，方便立即查看ADC。
     */
    LQ_Gray_Scan();
    (void)LQ_Gray_UpdateNormalized();

    g_lq_gray_position = 0;
    g_lq_gray_steering = 0;
    g_lq_gray_strength_sum = 0U;
    g_lq_gray_line_detected = false;
    g_lq_gray_center_four_black = false;

    s_last_control_ms = Clock_GetMs();

    s_left_target_command = 0;
    s_right_target_command = 0;
}

void Gray_Track_ResetControl(void)//复位函数
{
    PID_Reset(&s_line_pid);

    fre = 0U;

    g_lq_gray_position = 0;
    g_lq_gray_steering = 0;
    g_lq_gray_line_detected = false;
    g_lq_gray_center_four_black = false;

    s_left_target_command = 0;
    s_right_target_command = 0;

    s_last_control_ms = Clock_GetMs();
}


/*
 * 每10ms更新一次ADC和巡线PID。
 */
static void LQ_Gray_UpdateLineControl(uint8_t current_task)
{
    bool calibration_valid;
    int32_t position;
    int32_t steering;
    int32_t left_target;
    int32_t right_target;
    int32_t base_speed;
    float pid_output;

    /*
     * task2使用高速参数；
     * task4、task5、task6使用低速参数。
     */
    if (current_task == 2U)
    {
        base_speed = LINE_BASE_SPEED_TASK2;
    }
    else
    {
        base_speed = LINE_BASE_SPEED_TASK456;
    }

    LQ_Gray_Scan();

    calibration_valid =
        LQ_Gray_UpdateNormalized();
    g_lq_gray_center_four_black =
        calibration_valid &&
        LQ_Gray_CenterFourAreBlack();
    /*
     * 标定阶段或标定无效时停车。
     */
    if ((LQ_GRAY_TRACK_RUN_ENABLE == 0U) ||
        (!calibration_valid))
    {
        PID_Reset(&s_line_pid);

        fre = 0U;

        g_lq_gray_position = 0;
        g_lq_gray_steering = 0;
        g_lq_gray_strength_sum = 0U;
        g_lq_gray_line_detected = false;
        g_lq_gray_center_four_black = false;

        s_left_target_command = 0;
        s_right_target_command = 0;

        return;
    }

    position = 0;

    g_lq_gray_line_detected =
        LQ_Gray_CalculatePosition(
            &position);

    /*
     * 丢线处理。
     *
     * 这里保持你原来全白时继续直行的行为。
     */
    if (!g_lq_gray_line_detected)
    {
        if (fre < UINT16_MAX)
        {
            fre++;
        }

        if (fre == 20U)
        {
            /*
             * 后续需要丢线事件时在这里置标志位。
             */
        }

        PID_Reset(&s_line_pid);

        g_lq_gray_position = 0;
        g_lq_gray_steering = 0;

        s_left_target_command =
            base_speed;

        s_right_target_command =
            base_speed;

        return;
    }

    fre = 0U;

    g_lq_gray_position =
        position;

    /*
     * PID目标位置为0。
     *
     * 线在左侧：
     * position < 0
     * PID输出 > 0
     * 左轮减速、右轮加速
     *
     * 线在右侧则相反。
     */
    pid_output =
        PID_Update(
            &s_line_pid,
            0.0f,
            (float)position);

    steering =
        LQ_Gray_FloatToInt32(
            pid_output);

    steering =
        LQ_Gray_ClampInt32(
            steering,
            -LINE_MAX_CORRECTION,
            LINE_MAX_CORRECTION);

    g_lq_gray_steering =
        steering;

    left_target =
        base_speed - steering;

    right_target =
        base_speed + steering;

    left_target =
        LQ_Gray_ClampInt32(
            left_target,
            LINE_TARGET_MIN,
            LINE_TARGET_MAX);

    right_target =
        LQ_Gray_ClampInt32(
            right_target,
            LINE_TARGET_MIN,
            LINE_TARGET_MAX);

    s_left_target_command =
        left_target;

    s_right_target_command =
        right_target;
}


void track(uint8_t current_task)
{
    uint32_t current_ms;

    current_ms = Clock_GetMs();

    if ((uint32_t)(
            current_ms - s_last_control_ms) <
        LQ_GRAY_TRACK_PERIOD_MS)
    {
        return;
    }

    s_last_control_ms = current_ms;

    LQ_Gray_UpdateLineControl(current_task);

    MotorControl_SetLeftRightTarget(
        s_left_target_command,
        s_right_target_command);
}

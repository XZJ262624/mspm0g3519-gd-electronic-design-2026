/**
****************************************************************************
* @file    main.c
* @author  XZJxsdss
* @date    2025_7_31
* @brief   LP_M0G3507_OLED
****************************************************************************
**/
#include "Board_Config.h"
#include "main.h"
#include "ISR.h"
#include "Encoder.h"
#include "Gyro.h"
#include "Wireless_UART.h"
#include "Buzzer.h"
#include "ti_msp_dl_config.h"
#include "ips200.h"
#include "task.h"

const volatile EncoderSample_t *g_encoder_sample_ptr;

#define LEFT_MOTOR_TEST_TARGET_COUNT     (1500)
#define RIGHT_MOTOR_TEST_TARGET_COUNT    (1500)
#define TELEMETRY_PERIOD_MS              (50U)//每50ms发送一次数据
#define TASK_SEND_PERIOD_MS              (10U)//每10ms发送一次数据

#define GRAY_TELEMETRY_SEND_RAW_ADC      (0U)

#define DISPLAY_LABEL_SCALE              (3U)
#define DISPLAY_TIME_SCALE               (4U)

#define TASK_TIME_X                      (0U)
#define TASK_TIME_Y                      (100U)
#define IPS200_FONT_CELL_WIDTH           (6U)

#define TELEMETRY_PERIOD_MS              (50U)
#define TASK_SEND_PERIOD_MS              (10U)

#define TASK_IDLE_NUMBER                 (7U)
#define TASK_ONE_SHOT_1_NUMBER           (8U)
#define TASK_ONE_SHOT_2_NUMBER           (9U)
#define TASK_ONE_SHOT_3_NUMBER           (10U)
#define TASK_ONE_SHOT_4_NUMBER           (11U)
#define TASK_ONE_SHOT_5_NUMBER           (12U)
#define TASK_ONE_SHOT_6_NUMBER           (13U)
#define TASK_ONE_SHOT_7_NUMBER           (14U)

volatile uint32_t g_wireless_tx_result;
static char s_telemetry_buffer[128];

static void Chassis_Stop(void)
{
    MotorControl_SetEnabled(false);
    Gray_Track_ResetControl();
}   

//标定阶段发送D1-D16ADC
static void SendMotorTelemetry(void)
{
    int32_t text_length;

#if (GRAY_TELEMETRY_SEND_RAW_ADC != 0U)
    text_length = snprintf(
        s_telemetry_buffer,
        sizeof(s_telemetry_buffer),
        "%u,%u,%u,%u,%u,%u,%u,%u,"
        "%u,%u,%u,%u,%u,%u,%u,%u\r\n",
        (unsigned int)g_lq_gray_adc[0],
        (unsigned int)g_lq_gray_adc[1],
        (unsigned int)g_lq_gray_adc[2],
        (unsigned int)g_lq_gray_adc[3],
        (unsigned int)g_lq_gray_adc[4],
        (unsigned int)g_lq_gray_adc[5],
        (unsigned int)g_lq_gray_adc[6],
        (unsigned int)g_lq_gray_adc[7],
        (unsigned int)g_lq_gray_adc[8],
        (unsigned int)g_lq_gray_adc[9],
        (unsigned int)g_lq_gray_adc[10],
        (unsigned int)g_lq_gray_adc[11],
        (unsigned int)g_lq_gray_adc[12],
        (unsigned int)g_lq_gray_adc[13],
        (unsigned int)g_lq_gray_adc[14],
        (unsigned int)g_lq_gray_adc[15]);
#else
    text_length = snprintf(
        s_telemetry_buffer,
        sizeof(s_telemetry_buffer),
        "%ld\r\n",
        (long)g_lq_gray_position);
#endif

    if ((text_length > 0) &&
        ((uint32_t)text_length <
         sizeof(s_telemetry_buffer)))
    {
        g_wireless_tx_result =
            wireless_uart_send_buffer(
                (const uint8_t *)s_telemetry_buffer,
                (uint32_t)text_length);
    }
}

static void DrawTaskTime(uint32_t total_ms)
{
    enum
    {
        TASK_TIME_CHAR_COUNT = 12
    };

    static char previous_text[TASK_TIME_CHAR_COUNT + 1U];
    static bool cache_valid = false;

    char time_text[TASK_TIME_CHAR_COUNT + 1U];

    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;
    uint32_t milliseconds;

    uint16_t cell_width;
    uint8_t index;

    hours = (total_ms / 3600000U) % 100U;
    minutes = (total_ms / 60000U) % 60U;
    seconds = (total_ms / 1000U) % 60U;
    milliseconds = total_ms % 1000U;

    time_text[0] = (char)('0' + (hours / 10U));
    time_text[1] = (char)('0' + (hours % 10U));
    time_text[2] = ':';

    time_text[3] = (char)('0' + (minutes / 10U));
    time_text[4] = (char)('0' + (minutes % 10U));
    time_text[5] = ':';

    time_text[6] = (char)('0' + (seconds / 10U));
    time_text[7] = (char)('0' + (seconds % 10U));
    time_text[8] = '.';

    time_text[9] =
        (char)('0' + (milliseconds / 100U));

    time_text[10] =
        (char)('0' +
               ((milliseconds / 10U) % 10U));

    time_text[11] =
        (char)('0' + (milliseconds % 10U));

    time_text[12] = '\0';

    cell_width =
        (uint16_t)(
            IPS200_FONT_CELL_WIDTH *
            DISPLAY_TIME_SCALE);

    for (index = 0U;
         index < TASK_TIME_CHAR_COUNT;
         index++)
    {
        if ((!cache_valid) ||
            (time_text[index] != previous_text[index]))
        {
            IPS200_DrawChar(
                (uint16_t)(
                    TASK_TIME_X +
                    ((uint16_t)index * cell_width)),
                TASK_TIME_Y,
                time_text[index],
                IPS200_COLOR_GREEN,
                IPS200_COLOR_BLACK,
                DISPLAY_TIME_SCALE);
        }

        previous_text[index] = time_text[index];
    }

    previous_text[TASK_TIME_CHAR_COUNT] = '\0';
    cache_valid = true;
}

int main(void)
{
    bool stop_line_latched = false;
    bool task_confirmed;
    bool task_finished = false;
    bool task_running = false;
    bool task_send_active = false;

    /* 任务计时控制 */
    bool previous_task_confirmed = false;
    bool task_timer_running = false;

    uint32_t task_start_ms = 0U;
    uint32_t task_elapsed_ms = 0U;
    uint32_t last_task_displayed_ms = UINT32_MAX;

    uint8_t current_task;
    uint8_t last_displayed_task;
    uint8_t previous_selected_task;

    uint32_t last_displayed_ms = UINT32_MAX;
    uint32_t runtime_start_ms;
    uint32_t elapsed_ms;
    uint32_t last_telemetry_ms;
    uint32_t last_task_send_ms;
    uint32_t current_ms;

    EncoderSample_t task_encoder_sample;
    // uint8_t current_task;
    // uint8_t last_displayed_task;
    // uint32_t last_displayed_ms = UINT32_MAX;
    // uint32_t runtime_start_ms;
    // uint32_t elapsed_ms;
    
    // uint32_t last_telemetry_ms;
    // uint32_t last_task_send_ms;
    // uint32_t current_ms;

    SYSCFG_DL_init();

    MotorControl_SetLeftTarget(0);
    MotorControl_SetRightTarget(0);
    MotorControl_SetEnabled(false);

    Gray_Track_Init();
    Buzzer_Init();

    if (!Gyro_Init())
    {
        while (1);
    }

    if (wireless_uart_init() != 0U)
    {
        while (1);
    }

    if (!Task_Init())
    {
        while (1);
    }

    IPS200_Init();
    IPS200_Fill(IPS200_COLOR_BLACK);

    /* 上电运行时间标题 */
    IPS200_DrawString(
        0U,
        0U,
        "RUNTIME",
        IPS200_COLOR_CYAN,
        IPS200_COLOR_BLACK,
        DISPLAY_LABEL_SCALE);

    /* 上电运行时间 */
    IPS200_DrawRuntime(
        0U,
        28U,
        0U,
        IPS200_COLOR_GREEN,
        IPS200_COLOR_BLACK,
        DISPLAY_TIME_SCALE);

    /* 当前任务 */
    IPS200_DrawString(
        0U,
        68U,
        "TASK",
        IPS200_COLOR_CYAN,
        IPS200_COLOR_BLACK,
        DISPLAY_LABEL_SCALE);

    IPS200_DrawInt(
        90U,
        68U,
        (int32_t)g_current_task,
        IPS200_COLOR_GREEN,
        IPS200_COLOR_BLACK,
        DISPLAY_LABEL_SCALE);

    IPS200_DrawString(
        126U,
        68U,
        "TIME",
        IPS200_COLOR_CYAN,
        IPS200_COLOR_BLACK,
        DISPLAY_LABEL_SCALE);

    DrawTaskTime(0U);

    last_displayed_task = g_current_task;
    previous_selected_task = g_current_task;

    SysTick_Init();

    Encoder_Init();
    MotorControl_Init();
    PWM_Init();

    MotorControl_SetLeftTarget(0);

    MotorControl_SetRightTarget(0);

    MotorControl_SetEnabled(false);

    ISR_Init();
    Timer_Init();

    __enable_irq();

    g_encoder_sample_ptr =
        Encoder_GetLatestSamplePtr();

    current_ms = Clock_GetMs();
    runtime_start_ms = current_ms;
    last_telemetry_ms = current_ms;
    last_task_send_ms = current_ms;

    while (1)
    {
        current_ms = Clock_GetMs();
        elapsed_ms = (uint32_t)(current_ms - runtime_start_ms);

        if (elapsed_ms != last_displayed_ms)
        {
            IPS200_DrawRuntime(
                0U,
                28U,
                elapsed_ms,
                IPS200_COLOR_GREEN,
                IPS200_COLOR_BLACK,
                DISPLAY_TIME_SCALE);

            last_displayed_ms = elapsed_ms;
        }

        if ((uint32_t)(
            current_ms - last_telemetry_ms) >=
            TELEMETRY_PERIOD_MS)
        {
            last_telemetry_ms = current_ms;

            SendMotorTelemetry();
        }
        
        current_task = g_current_task;
        task_confirmed = g_task_confirmed;
        /*
        * task8、task9为一次性发送任务。
        * 不启动电机、不计时，也不进入周期发送逻辑。
        */
        if (task_confirmed &&
            (
            (current_task == TASK_ONE_SHOT_1_NUMBER) ||
            (current_task == TASK_ONE_SHOT_2_NUMBER) ||
            (current_task == TASK_ONE_SHOT_3_NUMBER) ||
            (current_task == TASK_ONE_SHOT_4_NUMBER) ||
            (current_task == TASK_ONE_SHOT_5_NUMBER) ||
            (current_task == TASK_ONE_SHOT_6_NUMBER) ||
            (current_task == TASK_ONE_SHOT_7_NUMBER)
            )
            )
        {
            Encoder_GetLatestSample(
                &task_encoder_sample);

            /*
            * Task_Send可能因为UART FIFO已满而暂时返回false。
            * 此时下一轮继续发送剩余字节，不会重新开始一帧。
            */
            if (Task_Send(
                    current_task,
                    (int16_t)task_encoder_sample.left_count,
                    (int16_t)task_encoder_sample.right_count))
            {
                /*
                * 发送完成后取消确认：
                * task8、task9返回task7；
                * task10、task11、task12返回task5。
                */
                g_task_confirmed = false;

                if ((current_task == TASK_ONE_SHOT_1_NUMBER) ||
                    (current_task == TASK_ONE_SHOT_2_NUMBER))
                {
                    g_current_task = TASK_IDLE_NUMBER;
                }
                else if((current_task == TASK_ONE_SHOT_3_NUMBER) ||
                    (current_task == TASK_ONE_SHOT_4_NUMBER))
                {
                    g_current_task = 5U;
                }
                else {
                     g_current_task = 6U;
                }
            }

            /*
            * 防止task8、task9进入电机、计时及周期发送逻辑。
            */
            continue;
        }

        if (current_task != previous_selected_task)
        {
            previous_selected_task = current_task;

            stop_line_latched = false;
            task_finished = false;
            task_running = false;
            task_send_active = false;

            if (task_timer_running)
            {
                task_elapsed_ms =
                    (uint32_t)(
                        current_ms - task_start_ms);
            }

            task_timer_running = false;
            last_task_displayed_ms = UINT32_MAX;
            previous_task_confirmed = false;

            Task_ResetSend();
            last_task_send_ms = current_ms;

            Chassis_Stop();
        }
        else
        {
            if (task_confirmed &&
                !previous_task_confirmed &&
                !task_finished)
            {
                task_start_ms = current_ms;
                task_elapsed_ms = 0U;
                task_timer_running = true;
                last_task_displayed_ms = UINT32_MAX;
            }

            previous_task_confirmed = task_confirmed;
        }

        /* 更新屏幕任务号 */
        if (current_task != last_displayed_task)
        {
            /*
            * 先清除两位任务号区域，
            * 避免12变成5以后残留数字2。
            */
            IPS200_FillRect(
                90U,
                68U,
                36U,
                24U,
                IPS200_COLOR_BLACK);

            IPS200_DrawInt(
                90U,
                68U,
                (int32_t)current_task,
                IPS200_COLOR_GREEN,
                IPS200_COLOR_BLACK,
                DISPLAY_LABEL_SCALE);

            last_displayed_task = current_task;
        }

        /* 当前任务还没有按SW2确认 */
        if (!task_confirmed)
        {
            if (task_running || MotorControl_IsEnabled())
            {
                Chassis_Stop();
            }

            task_running = false;
        }
        /*
        * task1、task3电机也不启动。
        */
        else if ((current_task == 1U) 
            ||(current_task == 3U)
            ||(current_task == 7U))
        {
            if (task_running || MotorControl_IsEnabled())
            {
                Chassis_Stop();
            }

            task_running = false;
        }
        /*
         * task2、task4、task5、task6按SW2确认后启动巡线。
         */
        else if (!stop_line_latched)
        {
            if (!task_running)
            {
                Gray_Track_ResetControl();

                /*
                * task2不使用启动Ramp；
                * 只有task4、task5、task6使用。
                */
                MotorControl_SetStartRampEnabled(
                    (current_task >= 4U) &&
                    (current_task <= 6U));

                MotorControl_SetEnabled(true);

                task_running = true;
            }

            track(current_task);

            /*
            * task2、task4、task5、task6到达终点时都停止计时。
            *
            * task2：停车并结束任务。
            * task4～6：不停车，继续巡线
            */
            if (g_lq_gray_center_four_black)
            {
                /* 所有巡线任务到达终点后冻结任务时间 */
                if (task_timer_running)
                {
                    task_elapsed_ms =
                        (uint32_t)(
                            current_ms - task_start_ms);

                    task_timer_running = false;
                    last_task_displayed_ms = UINT32_MAX;
                }

                /*
                * 只有task2停车。
                * task4～6不修改task_running和stop_line_latched，
                * 所以下一轮循环仍会继续执行track()。
                */
                if (current_task == 2U)
                {
                    Chassis_Stop();

                    task_running = false;
                    stop_line_latched = true;
                    task_finished = true;
                }
            }
        }

        /* 任务运行期间更新任务用时 */
        if (task_timer_running)
        {
            task_elapsed_ms =
                (uint32_t)(
                    current_ms - task_start_ms);
        }

        /* 只在时间变化时更新屏幕 */
        if (task_elapsed_ms != last_task_displayed_ms)
        {
            DrawTaskTime(task_elapsed_ms);
            last_task_displayed_ms = task_elapsed_ms;
        }

        /*
         * 按下SW2确认后启动发送计时。
         * 确认后的第20ms发送第一帧，
         * 之后每20ms发送一帧。
         */
        if (task_confirmed && !task_finished)
        {
            if (!task_send_active)
            {
                Task_ResetSend();
                last_task_send_ms = current_ms;
                task_send_active = true;
            }

            if ((uint32_t)(
                current_ms - last_task_send_ms) >=
                TASK_SEND_PERIOD_MS)
            {
                Encoder_GetLatestSample(
                    &task_encoder_sample);

                if (Task_Send(
                        current_task,
                        (int16_t)task_encoder_sample.left_count,
                        (int16_t)task_encoder_sample.right_count))
                {
                    last_task_send_ms = current_ms;
                }
            }
        }
        else if (task_send_active)
        {
            Task_ResetSend();

            task_send_active = false;
            last_task_send_ms = current_ms;
        }
    }
}
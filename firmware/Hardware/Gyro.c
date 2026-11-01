#include "Gyro.h"
#include "UART.h"
#include "Board_Config.h"

#define GYRO_FRAME_SIZE    (5U)
#define GYRO_FRAME_HEADER  (0x5AU)
#define GYRO_TYPE_RATE_Z   (0xAAU)
#define GYRO_TYPE_YAW      (0xBBU)

volatile float g_gyro_rate_z_dps = 0.0f;
volatile float g_gyro_yaw_deg = 0.0f;
volatile uint32_t g_gyro_frame_count = 0U;

static uint8_t s_rx_frame[GYRO_FRAME_SIZE];
static uint8_t s_rx_index = 0U;

static void Gyro_ParseByte(uint8_t data)
{
    uint8_t checksum;
    int16_t raw_data;

    /* 等待帧头0x5A */
    if (s_rx_index == 0U)
    {
        if (data == GYRO_FRAME_HEADER)
        {
            s_rx_frame[0] = data;
            s_rx_index = 1U;
        }

        return;
    }

    s_rx_frame[s_rx_index] = data;
    s_rx_index++;

    if (s_rx_index < GYRO_FRAME_SIZE)
    {
        return;
    }

    s_rx_index = 0U;

    checksum = (uint8_t)(
        s_rx_frame[0] +
        s_rx_frame[1] +
        s_rx_frame[2] +
        s_rx_frame[3]);

    if (checksum != s_rx_frame[4])
    {
        return;
    }

    raw_data = (int16_t)(
        ((uint16_t)s_rx_frame[3] << 8) |
        (uint16_t)s_rx_frame[2]);

    if (s_rx_frame[1] == GYRO_TYPE_RATE_Z)
    {
        g_gyro_rate_z_dps =
            (float)raw_data / 32768.0f * 2000.0f;

        g_gyro_frame_count++;
    }
    else if (s_rx_frame[1] == GYRO_TYPE_YAW)
    {
        g_gyro_yaw_deg =
            (float)raw_data / 32768.0f * 180.0f;

        g_gyro_frame_count++;
    }
}

static void Gyro_RxCallback(
    UART_Port_t port,
    uint8_t data)
{
    if (port == BSP_GYRO_UART_PORT)
    {
        Gyro_ParseByte(data);
    }
}

bool Gyro_Init(void)
{
    s_rx_index = 0U;
    g_gyro_rate_z_dps = 0.0f;
    g_gyro_yaw_deg = 0.0f;
    g_gyro_frame_count = 0U;

    if (!UART_AttachConfigured(BSP_GYRO_UART_PORT))
    {
        return false;
    }

    UART_SetRxCallback(
        BSP_GYRO_UART_PORT,
        Gyro_RxCallback);

    return UART_EnableRxInterrupt(
        BSP_GYRO_UART_PORT,
        BSP_GYRO_UART_IRQ_PRIORITY);
}

float Gyro_GetRateZ(void)
{
    return g_gyro_rate_z_dps;
}

float Gyro_GetYaw(void)
{
    return g_gyro_yaw_deg;
}
#include "Wireless_UART.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "Board_Config.h"
#include "UART.h"

static volatile uint8_t s_rx_buffer[
    WIRELESS_UART_BUFFER_SIZE];

static volatile uint16_t s_rx_head;
static volatile uint16_t s_rx_tail;
static volatile uint16_t s_rx_count;

static bool WirelessUART_IsModuleBusy(void)
{
    uint32_t level;

    level = DL_GPIO_readPins(
        BSP_WIRELESS_UART_RTS_PORT,
        BSP_WIRELESS_UART_RTS_PIN);

    /*
     * 逐飞无线串口：
     * RTS低电平表示允许发送；
     * RTS高电平表示模块忙。
     */
    return (
        (level & BSP_WIRELESS_UART_RTS_PIN) != 0U);
}

static bool WirelessUART_WaitReady(void)
{
    uint32_t timeout_count;

    for (timeout_count = 0U;
         timeout_count < WIRELESS_UART_TIMEOUT_COUNT;
         timeout_count++)
    {
        if (!WirelessUART_IsModuleBusy())
        {
            return true;
        }

        /*
         * 约等待1ms。
         * 不依赖SysTick，因此即使启动阶段发送也不会卡住。
         */
        delay_cycles(CPUCLK_FREQ / 1000U);
    }

    return false;
}

static void WirelessUART_RxCallback(
    UART_Port_t port,
    uint8_t data)
{
    if (port != BSP_WIRELESS_UART_PORT)
    {
        return;
    }

    if (s_rx_count >= WIRELESS_UART_BUFFER_SIZE)
    {
        /*
         * 接收缓冲区已满，丢弃新数据。
         * 行为与逐飞FIFO空间不足时一致。
         */
        return;
    }

    s_rx_buffer[s_rx_head] = data;

    s_rx_head++;

    if (s_rx_head >= WIRELESS_UART_BUFFER_SIZE)
    {
        s_rx_head = 0U;
    }

    s_rx_count++;
}

uint8_t wireless_uart_init(void)
{
    uint32_t interrupt_state;

    interrupt_state = __get_PRIMASK();
    __disable_irq();

    s_rx_head = 0U;
    s_rx_tail = 0U;
    s_rx_count = 0U;

    if (interrupt_state == 0U)
    {
        __enable_irq();
    }

    /*
     * UART1硬件由SYSCFG_DL_init()配置，
     * 所以这里只接管，不重新初始化寄存器。
     */
    if (!UART_AttachConfigured(
            BSP_WIRELESS_UART_PORT))
    {
        return 1U;
    }

    UART_SetRxCallback(
        BSP_WIRELESS_UART_PORT,
        WirelessUART_RxCallback);

    if (!UART_EnableRxInterrupt(
            BSP_WIRELESS_UART_PORT,
            BSP_WIRELESS_UART_IRQ_PRIORITY))
    {
        UART_SetRxCallback(
            BSP_WIRELESS_UART_PORT,
            NULL);

        return 1U;
    }

    return 0U;
}

uint32_t wireless_uart_send_byte(uint8_t data)
{
    if (!WirelessUART_WaitReady())
    {
        return 1U;
    }

    if (!UART_WriteByte(
            BSP_WIRELESS_UART_PORT,
            data))
    {
        return 1U;
    }

    return 0U;
}

uint32_t wireless_uart_send_buffer(
    const uint8_t *buffer,
    uint32_t length)
{
    uint32_t send_length;

    if ((buffer == NULL) && (length != 0U))
    {
        return length;
    }

    while (length != 0U)
    {
        /*
         * 逐飞原库每次最多发送30字节，
         * 每包发送前重新检查RTS。
         */
        if (!WirelessUART_WaitReady())
        {
            break;
        }

        if (length >= WIRELESS_UART_SEND_CHUNK_SIZE)
        {
            send_length =
                WIRELESS_UART_SEND_CHUNK_SIZE;
        }
        else
        {
            send_length = length;
        }

        if (!UART_WriteBuffer(
                BSP_WIRELESS_UART_PORT,
                buffer,
                send_length))
        {
            break;
        }

        buffer += send_length;
        length -= send_length;
    }

    return length;
}

uint32_t wireless_uart_send_string(
    const char *string)
{
    size_t length;

    if (string == NULL)
    {
        return 1U;
    }

    length = strlen(string);

    return wireless_uart_send_buffer(
        (const uint8_t *)string,
        (uint32_t)length);
}

uint32_t wireless_uart_read_buffer(
    uint8_t *buffer,
    uint32_t length)
{
    uint32_t read_length;
    uint32_t interrupt_state;

    if ((buffer == NULL) || (length == 0U))
    {
        return 0U;
    }

    read_length = 0U;

    /*
     * 接收回调在UART1中断中写FIFO。
     * 读取期间短暂关闭中断，避免FIFO索引竞争。
     */
    interrupt_state = __get_PRIMASK();
    __disable_irq();

    while ((read_length < length) &&
           (s_rx_count != 0U))
    {
        buffer[read_length] =
            s_rx_buffer[s_rx_tail];

        read_length++;
        s_rx_tail++;

        if (s_rx_tail >=
            WIRELESS_UART_BUFFER_SIZE)
        {
            s_rx_tail = 0U;
        }

        s_rx_count--;
    }

    if (interrupt_state == 0U)
    {
        __enable_irq();
    }

    return read_length;
}

uint32_t wireless_uart_rx_used(void)
{
    uint32_t used;
    uint32_t interrupt_state;

    interrupt_state = __get_PRIMASK();
    __disable_irq();

    used = s_rx_count;

    if (interrupt_state == 0U)
    {
        __enable_irq();
    }

    return used;
}
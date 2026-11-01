#include "UART.h"

#define UART_POWER_STARTUP_DELAY_CYCLES    (16U)//上电后等待周期，让外设电源状态稳定
//RXFIFO阈值设置为空
#define UART_DEFAULT_RX_FIFO_LEVEL         \
    DL_UART_MAIN_RX_FIFO_LEVEL_ONE_ENTRY
//TXFIFO阈值设置为空
#define UART_DEFAULT_TX_FIFO_LEVEL         \
    DL_UART_MAIN_TX_FIFO_LEVEL_EMPTY

typedef struct
{
    UART_Regs *instance;//外设寄存器基地址,如UART0,UART1,UART4
    IRQn_Type irqn;//UART1_INT_IRQn,用于操作NVIC
    uint32_t clock_frequency_hz;//输入时钟频率，用于计算波特率分频参数
} UART_HardwareConfig_t;

//UART硬件配置表
static const UART_HardwareConfig_t s_uart_hardware[UART_PORT_COUNT] =
{
    {
        BSP_UART0_INST,
        BSP_UART0_IRQN,
        BSP_UART0_CLOCK_FREQ_HZ
    },
    {
        BSP_UART1_INST,
        BSP_UART1_IRQN,
        BSP_UART1_CLOCK_FREQ_HZ
    },
    {
        BSP_UART3_INST,
        BSP_UART3_IRQN,
        BSP_UART3_CLOCK_FREQ_HZ
    },
    {
        BSP_UART4_INST,
        BSP_UART4_IRQN,
        BSP_UART4_CLOCK_FREQ_HZ
    },
    {
        BSP_UART5_INST,
        BSP_UART5_IRQN,
        BSP_UART5_CLOCK_FREQ_HZ
    },
    {
        BSP_UART6_INST,
        BSP_UART6_IRQN,
        BSP_UART6_CLOCK_FREQ_HZ
    },
    {
        BSP_UART7_INST,
        BSP_UART7_IRQN,
        BSP_UART7_CLOCK_FREQ_HZ
    }
};

//回调数组和初始化数组
// s_rx_callback[0] = UART0 回调
// s_rx_callback[2] = UART3 回调
// s_initialized[0] = UART0 是否初始化
// s_initialized[1] = UART1 是否初始化
static UART_RxCallback_t s_rx_callback[UART_PORT_COUNT];
static bool s_initialized[UART_PORT_COUNT];

//端口有效性检查,防止数组越界合法范围：0-6
static bool UART_IsPortValid(UART_Port_t port)
{
    return ((uint32_t)port < (uint32_t)UART_PORT_COUNT);
}

UART_Regs *UART_GetInstance(UART_Port_t port)
{
    if (!UART_IsPortValid(port))
    {
        return NULL;
    }

    return s_uart_hardware[port].instance;
}

IRQn_Type UART_GetIRQn(UART_Port_t port)
{
    if (!UART_IsPortValid(port))
    {
        return (IRQn_Type)0;
    }

    return s_uart_hardware[port].irqn;
}

uint32_t UART_GetClockFrequency(UART_Port_t port)
{
    if (!UART_IsPortValid(port))
    {
        return 0U;
    }

    return s_uart_hardware[port].clock_frequency_hz;
}

//UART_Init(UART_PORT_1, 115200U);
//需要手动配置TX/RX引脚复用
// UART_SetRxCallback(
//     UART_PORT_1,
//     WirelessRxCallback);
// 需要手动开启RX中断
// UART_EnableRxInterrupt(
//     UART_PORT_1,
//     2U);
bool UART_Init(UART_Port_t port, uint32_t baud_rate)
{
    UART_Regs *instance;//寄存器实例
    uint32_t clock_frequency_hz;//输入时钟频率

    //UART时钟配置
    static const DL_UART_Main_ClockConfig clock_config =
    {
        .clockSel = DL_UART_MAIN_CLOCK_BUSCLK,//时钟源使用BUSCLK
        .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1//不分频
    };

    //UART帧格式配置
    static const DL_UART_Main_Config uart_config =
    {
        .mode = DL_UART_MAIN_MODE_NORMAL,//普通 UART 模式
        .direction = DL_UART_MAIN_DIRECTION_TX_RX,//同时允许发送和接收
        .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,//不使用 CTS/RTS 硬件流控
        .parity = DL_UART_MAIN_PARITY_NONE,//无校验
        .wordLength = DL_UART_MAIN_WORD_LENGTH_8_BITS,//8 位数据
        .stopBits = DL_UART_MAIN_STOP_BITS_ONE//1 位停止位
    };

    if (!UART_IsPortValid(port) || (baud_rate == 0U))
    {
        return false;
    }

    instance = s_uart_hardware[port].instance;
    clock_frequency_hz =
        s_uart_hardware[port].clock_frequency_hz;

    //初始化前关闭NVIC
    NVIC_DisableIRQ(s_uart_hardware[port].irqn);
    NVIC_ClearPendingIRQ(s_uart_hardware[port].irqn);

    //复位上电
    DL_UART_Main_reset(instance);
    DL_UART_Main_enablePower(instance);
    delay_cycles(UART_POWER_STARTUP_DELAY_CYCLES);//等待

    DL_UART_Main_setClockConfig(instance, &clock_config);
    DL_UART_Main_init(instance, &uart_config);

    //设置16倍过采样
    DL_UART_Main_setOversampling(
        instance,
        DL_UART_MAIN_OVERSAMPLING_RATE_16X);

    //配置波特率
    DL_UART_Main_configBaudRate(
        instance,
        clock_frequency_hz,
        baud_rate);

    //开启FIFO
    DL_UART_Main_enableFIFOs(instance);
    //设置FIFO阈值
    DL_UART_Main_setRXFIFOThreshold(
        instance,
        UART_DEFAULT_RX_FIFO_LEVEL);
    DL_UART_Main_setTXFIFOThreshold(
        instance,
        UART_DEFAULT_TX_FIFO_LEVEL);

    DL_UART_Main_disableInterrupt(
        instance,
        DL_UART_MAIN_INTERRUPT_RX |
        DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR |
        DL_UART_MAIN_INTERRUPT_BREAK_ERROR |
        DL_UART_MAIN_INTERRUPT_PARITY_ERROR |
        DL_UART_MAIN_INTERRUPT_FRAMING_ERROR |
        DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR |
        DL_UART_MAIN_INTERRUPT_NOISE_ERROR);

    DL_UART_Main_enable(instance);
    s_initialized[port] = true;

    return true;
}

void UART_DeInit(UART_Port_t port)
{
    UART_Regs *instance;

    if (!UART_IsPortValid(port))
    {
        return;
    }

    instance = s_uart_hardware[port].instance;

    NVIC_DisableIRQ(s_uart_hardware[port].irqn);
    NVIC_ClearPendingIRQ(s_uart_hardware[port].irqn);

    DL_UART_Main_disableInterrupt(
        instance,
        DL_UART_MAIN_INTERRUPT_RX |
        DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR |
        DL_UART_MAIN_INTERRUPT_BREAK_ERROR |
        DL_UART_MAIN_INTERRUPT_PARITY_ERROR |
        DL_UART_MAIN_INTERRUPT_FRAMING_ERROR |
        DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR |
        DL_UART_MAIN_INTERRUPT_NOISE_ERROR);

    DL_UART_Main_disable(instance);
    DL_UART_Main_reset(instance);
    DL_UART_Main_disablePower(instance);

    s_rx_callback[port] = NULL;
    s_initialized[port] = false;
}

bool UART_IsInitialized(UART_Port_t port)
{
    if (!UART_IsPortValid(port))
    {
        return false;
    }

    return s_initialized[port];
}

// 接收中断接口
// 开启UART外设内部的中断源
// 开启NVIC中的UART中断通道
// 该函数只有在UART已初始化后才能成功
bool UART_EnableRxInterrupt(UART_Port_t port, uint32_t priority)
{
    UART_Regs *instance;

    if (!UART_IsPortValid(port) || !s_initialized[port])
    {
        return false;
    }

    instance = s_uart_hardware[port].instance;

    DL_UART_Main_enableInterrupt(
        instance,
        DL_UART_MAIN_INTERRUPT_RX |
        DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR |
        DL_UART_MAIN_INTERRUPT_BREAK_ERROR |
        DL_UART_MAIN_INTERRUPT_PARITY_ERROR |
        DL_UART_MAIN_INTERRUPT_FRAMING_ERROR |
        DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR |
        DL_UART_MAIN_INTERRUPT_NOISE_ERROR);

    NVIC_ClearPendingIRQ(s_uart_hardware[port].irqn);
    NVIC_SetPriority(s_uart_hardware[port].irqn, priority);
    NVIC_EnableIRQ(s_uart_hardware[port].irqn);

    return true;
}

void UART_DisableRxInterrupt(UART_Port_t port)
{
    if (!UART_IsPortValid(port))
    {
        return;
    }

    NVIC_DisableIRQ(s_uart_hardware[port].irqn);

    DL_UART_Main_disableInterrupt(
        s_uart_hardware[port].instance,
        DL_UART_MAIN_INTERRUPT_RX |
        DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR |
        DL_UART_MAIN_INTERRUPT_BREAK_ERROR |
        DL_UART_MAIN_INTERRUPT_PARITY_ERROR |
        DL_UART_MAIN_INTERRUPT_FRAMING_ERROR |
        DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR |
        DL_UART_MAIN_INTERRUPT_NOISE_ERROR);

    NVIC_ClearPendingIRQ(s_uart_hardware[port].irqn);
}

// UART_SetRxCallback(
//     UART_PORT_1,
//     MyUartRxCallback);
// UART1收到数据后，中断程序会调用：
// MyUartRxCallback(UART_PORT_1, data);
void UART_SetRxCallback(
    UART_Port_t port,
    UART_RxCallback_t callback)
{
    if (!UART_IsPortValid(port))
    {
        return;
    }

    s_rx_callback[port] = callback;
}

void UART_IRQHandler(UART_Port_t port)
{
    UART_Regs *instance;
    uint32_t interrupt_index;

    if (!UART_IsPortValid(port))
    {
        return;
    }

    instance = s_uart_hardware[port].instance;
    interrupt_index = DL_UART_Main_getPendingInterrupt(instance);

    switch (interrupt_index)
    {
        case DL_UART_MAIN_IIDX_RX:
            while (!DL_UART_Main_isRXFIFOEmpty(instance))
            {
                uint8_t data = DL_UART_Main_receiveData(instance);

                if (s_rx_callback[port] != NULL)
                {
                    s_rx_callback[port](port, data);
                }
            }
            break;

        case DL_UART_MAIN_IIDX_OVERRUN_ERROR:
        case DL_UART_MAIN_IIDX_BREAK_ERROR:
        case DL_UART_MAIN_IIDX_PARITY_ERROR:
        case DL_UART_MAIN_IIDX_FRAMING_ERROR:
        case DL_UART_MAIN_IIDX_RX_TIMEOUT_ERROR:
        case DL_UART_MAIN_IIDX_NOISE_ERROR:

            while (!DL_UART_Main_isRXFIFOEmpty(instance))
            {
                (void)DL_UART_Main_receiveData(instance);
            }
            break;

        default:
            break;
    }
}

bool UART_WriteByte(UART_Port_t port, uint8_t data)
{
    if (!UART_IsPortValid(port) || !s_initialized[port])
    {
        return false;
    }

    DL_UART_Main_transmitDataBlocking(
        s_uart_hardware[port].instance,
        data);

    return true;
}

bool UART_WriteBuffer(
    UART_Port_t port,
    const uint8_t *data,
    size_t length)
{
    size_t index;

    if (!UART_IsPortValid(port) ||
        !s_initialized[port] ||
        ((data == NULL) && (length != 0U)))
    {
        return false;
    }

    for (index = 0U; index < length; index++)
    {
        DL_UART_Main_transmitDataBlocking(
            s_uart_hardware[port].instance,
            data[index]);
    }

    return true;
}

bool UART_WriteString(UART_Port_t port, const char *string)
{
    if (string == NULL)
    {
        return false;
    }

    while (*string != '\0')
    {
        if (!UART_WriteByte(port, (uint8_t)*string))
        {
            return false;
        }

        string++;
    }

    return true;
}

bool UART_TryWriteByte(
    UART_Port_t port,
    uint8_t data)
{
    UART_Regs *instance;

    if (!UART_IsPortValid(port) ||
        !s_initialized[port])
    {
        return false;
    }

    instance = s_uart_hardware[port].instance;

    /*
     * 硬件 TX FIFO 已满时立即返回，
     * 不使用 transmitDataBlocking() 等待。
     */
    if (DL_UART_Main_isTXFIFOFull(instance))
    {
        return false;
    }

    /*
     * transmitData() 本身不等待，
     * 所以调用前必须先检查 FIFO 是否已满。
     */
    DL_UART_Main_transmitData(
        instance,
        data);

    return true;
}

bool UART_IsTxIdle(UART_Port_t port)
{
    if (!UART_IsPortValid(port) ||
        !s_initialized[port])
    {
        return false;
    }

    /*
     * BUSY 在 TX FIFO、TXDATA 或移位寄存器仍有数据时为 true。
     */
    return !DL_UART_Main_isBusy(
        s_uart_hardware[port].instance);
}

bool UART_TryReadByte(UART_Port_t port, uint8_t *data)
{
    UART_Regs *instance;

    if (!UART_IsPortValid(port) ||
        !s_initialized[port] ||
        (data == NULL))
    {
        return false;
    }

    instance = s_uart_hardware[port].instance;

    if (DL_UART_Main_isRXFIFOEmpty(instance))
    {
        return false;
    }

    *data = DL_UART_Main_receiveData(instance);
    return true;
}

bool UART_AttachConfigured(
    UART_Port_t port)
{
    UART_Regs *instance;

    if (!UART_IsPortValid(port))
    {
        return false;
    }

    instance =
        s_uart_hardware[port].instance;

    /*
     * SysConfig 中不启用 UART 中断。
     * 此处再次关闭，确保注册回调之前不会进入中断。
     */
    NVIC_DisableIRQ(
        s_uart_hardware[port].irqn);

    NVIC_ClearPendingIRQ(
        s_uart_hardware[port].irqn);

    DL_UART_Main_disableInterrupt(
        instance,
        DL_UART_MAIN_INTERRUPT_RX |
        DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR |
        DL_UART_MAIN_INTERRUPT_BREAK_ERROR |
        DL_UART_MAIN_INTERRUPT_PARITY_ERROR |
        DL_UART_MAIN_INTERRUPT_FRAMING_ERROR |
        DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR |
        DL_UART_MAIN_INTERRUPT_NOISE_ERROR);

    s_rx_callback[port] = NULL;
    s_initialized[port] = true;

    return true;
}

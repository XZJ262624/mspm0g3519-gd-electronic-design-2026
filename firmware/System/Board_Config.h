#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "ti_msp_dl_config.h"
/*
 * AT8236双路驱动
 *
 * 左轮：
 * IN1 = PB3  / TIMA0 CC0
 * IN2 = PB12 / TIMA0 CC2
 *
 * 右轮：
 * IN1 = PB4  / TIMA0 CC1
 * IN2 = PB13 / TIMA0 CC3
 */
#define BSP_MOTOR_PWM_INST                    MOTOR_PWM_INST

#define BSP_MOTOR_LEFT_IN1_PWM_CC_INDEX       DL_TIMER_CC_0_INDEX//PB3
#define BSP_MOTOR_LEFT_IN2_PWM_CC_INDEX       DL_TIMER_CC_2_INDEX//PB12

#define BSP_MOTOR_RIGHT_IN1_PWM_CC_INDEX      DL_TIMER_CC_3_INDEX//PB13
#define BSP_MOTOR_RIGHT_IN2_PWM_CC_INDEX      DL_TIMER_CC_1_INDEX//PB4


//双路硬件QEI
#define BSP_QEI_LEFT_INST \
    QEI_LEFT_INST

#define BSP_QEI_RIGHT_INST \
    QEI_RIGHT_INST

#define BSP_QEI_LEFT_SIGN   1
#define BSP_QEI_RIGHT_SIGN  -1

//速度闭环定时器
#define BSP_SPEED_TIMER_INST                  TIMER_G0_INST
#define BSP_SPEED_TIMER_IRQN                  TIMER_G0_INST_INT_IRQN
#define BSP_SPEED_TIMER_HANDLER               TIMER_G0_INST_IRQHandler

//按键扫描定时器
#define BSP_KEY_TIMER_INST                    TIMER_G7_INST
#define BSP_KEY_TIMER_IRQN                    TIMER_G7_INST_INT_IRQN
#define BSP_KEY_TIMER_HANDLER                 TIMER_G7_INST_IRQHandler

//按键接口
#define BSP_KEY_SW2_PORT                      BUTTON_SW2_PORT
#define BSP_KEY_SW2_PIN                       BUTTON_SW2_PIN

#define BSP_KEY_SW3_PORT                      BUTTON_SW3_PORT
#define BSP_KEY_SW3_PIN                       BUTTON_SW3_PIN

#define BSP_KEY_SW4_PORT                      BUTTON_SW4_PORT
#define BSP_KEY_SW4_PIN                       BUTTON_SW4_PIN

#define BSP_KEY_SW5_PORT                      BUTTON_SW5_PORT
#define BSP_KEY_SW5_PIN                       BUTTON_SW5_PIN

//蜂鸣器接口
#define BSP_BUZZER_PORT                       BUZZER_PORT
#define BSP_BUZZER_PIN                        BUZZER_CTL_PIN

/*
 * 16路模拟灰度模块（单ADC轮询）
 * ADC/AO -> PB25 / ADC0 Channel 4（唯一模拟输入）
 * S0     -> PB17（MUX地址位0）
 * S1     -> PB18（MUX地址位1）
 * S2     -> PB20（MUX地址位2）
 * S3     -> PB19（MUX地址位3）
 */
#define BSP_LQ_GRAY_ADC_INST \
    ADC_Tracking_INST

#define BSP_LQ_GRAY_ADC_MEM \
    DL_ADC12_MEM_IDX_0

#define BSP_LQ_GRAY_ADC_RESULT_INTERRUPT \
    DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED

#define BSP_LQ_GRAY_MUX_PORT    GPIOB

#define BSP_LQ_GRAY_S0_PIN      DL_GPIO_PIN_17
#define BSP_LQ_GRAY_S1_PIN      DL_GPIO_PIN_18
#define BSP_LQ_GRAY_S2_PIN      DL_GPIO_PIN_20
#define BSP_LQ_GRAY_S3_PIN      DL_GPIO_PIN_19

/*
 * UART interfaces
 *
 * MSPM0G3519 provides UART0, UART1, UART3, UART4, UART5, UART6 and UART7.
 * UART0/UART1/UART7 are in the MCLK peripheral domain. UART3-UART6 are in
 * the ULPCLK peripheral domain. The current clock tree uses MCLK = 80 MHz
 * and ULPCLK = MCLK / 2.
 *
 * These definitions do not initialize any UART or change any pin mux. They
 * only provide a single board-level mapping for the low-level UART driver.
 */
#define BSP_UART_PD0_CLOCK_FREQ_HZ       (CPUCLK_FREQ)
#define BSP_UART_PD1_CLOCK_FREQ_HZ       (CPUCLK_FREQ / 2U)

#define BSP_UART0_INST                   UART0
#define BSP_UART0_IRQN                   UART0_INT_IRQn
#define BSP_UART0_HANDLER                UART0_IRQHandler
#define BSP_UART0_CLOCK_FREQ_HZ          BSP_UART_PD0_CLOCK_FREQ_HZ

#define BSP_UART1_INST                   UART1
#define BSP_UART1_IRQN                   UART1_INT_IRQn
#define BSP_UART1_HANDLER                UART1_IRQHandler
#define BSP_UART1_CLOCK_FREQ_HZ          BSP_UART_PD0_CLOCK_FREQ_HZ

#define BSP_UART3_INST                   UART3
#define BSP_UART3_IRQN                   UART3_INT_IRQn
#define BSP_UART3_HANDLER                UART3_IRQHandler
#define BSP_UART3_CLOCK_FREQ_HZ          BSP_UART_PD1_CLOCK_FREQ_HZ

#define BSP_UART4_INST                   UART4
#define BSP_UART4_IRQN                   UART4_INT_IRQn
#define BSP_UART4_HANDLER                UART4_IRQHandler
#define BSP_UART4_CLOCK_FREQ_HZ          BSP_UART_PD1_CLOCK_FREQ_HZ

#define BSP_UART5_INST                   UART5
#define BSP_UART5_IRQN                   UART5_INT_IRQn
#define BSP_UART5_HANDLER                UART5_IRQHandler
#define BSP_UART5_CLOCK_FREQ_HZ          BSP_UART_PD1_CLOCK_FREQ_HZ

#define BSP_UART6_INST                   UART6
#define BSP_UART6_IRQN                   UART6_INT_IRQn
#define BSP_UART6_HANDLER                UART6_IRQHandler
#define BSP_UART6_CLOCK_FREQ_HZ          BSP_UART_PD1_CLOCK_FREQ_HZ

#define BSP_UART7_INST                   UART7
#define BSP_UART7_IRQN                   UART7_INT_IRQn
#define BSP_UART7_HANDLER                UART7_IRQHandler
#define BSP_UART7_CLOCK_FREQ_HZ          BSP_UART_PD0_CLOCK_FREQ_HZ

#define BSP_WIRELESS_UART_PORT           UART_PORT_1
#define BSP_WIRELESS_UART_IRQ_PRIORITY   (2U)

#define BSP_GYRO_UART_PORT               UART_PORT_7
#define BSP_GYRO_UART_IRQ_PRIORITY       (2U)

#define BSP_TASK_UART_PORT               UART_PORT_4
#define BSP_TASK_UART_IRQ_PRIORITY       (2U)

#define BSP_WIRELESS_UART_RTS_PORT       WIRELESS_CTRL_PORT
#define BSP_WIRELESS_UART_RTS_PIN        WIRELESS_CTRL_RTS_PIN

#endif

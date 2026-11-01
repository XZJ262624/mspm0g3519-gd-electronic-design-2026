#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Board_Config.h"
 
 //对外声明UART接口、端口枚举、回调函数类型
typedef enum
{
    UART_PORT_0 = 0,
    UART_PORT_1 = 1,
    UART_PORT_3 = 2,
    UART_PORT_4 = 3,
    UART_PORT_5 = 4,
    UART_PORT_6 = 5,
    UART_PORT_7 = 6,
    UART_PORT_COUNT = 7
} UART_Port_t;

//接收回调函数类型
typedef void (*UART_RxCallback_t)(UART_Port_t port, uint8_t data);

//初始化接口
bool UART_Init(UART_Port_t port, uint32_t baud_rate);
// 反初始化接口
// 关闭NVICUART中断,UART内部中断
// 关闭复位UART,关闭UART电源
// 清除接收回调以及initialized标志
void UART_DeInit(UART_Port_t port);

//初始化状态查询,用于判断对应UART是否已经调用过UART_Init
bool UART_IsInitialized(UART_Port_t port);

bool UART_EnableRxInterrupt(UART_Port_t port, uint32_t priority);
void UART_DisableRxInterrupt(UART_Port_t port);//只关闭中断，不会关闭UART外设

//注册接收回调,回调中不能做耗时操作
void UART_SetRxCallback(UART_Port_t port, UART_RxCallback_t callback);
// 例如使用
// static void WirelessRxCallback()
// {    (void)port;
//     FIFO_WriteByte(data);}
// 注册：
// UART_SetRxCallback(
//     UART_PORT_1,
//     WirelessRxCallback);

void UART_IRQHandler(UART_Port_t port);

//发送接口
//发送一个字节,阻塞发送
bool UART_WriteByte(UART_Port_t port, uint8_t data);
//尝试向UART硬件TXFIFO写入一个字节。
//true：
//字节已写入硬件 TX FIFO。
//false：
//UART未初始化、端口无效，或者硬件FIFO已满。
bool UART_TryWriteByte(
    UART_Port_t port,
    uint8_t data);
bool UART_IsTxIdle(UART_Port_t port);
//发送字节数组,阻塞发送
bool UART_WriteBuffer(
    UART_Port_t port,
    const uint8_t *data,
    size_t length);
//发送字符串,阻塞发送
bool UART_WriteString(UART_Port_t port, const char *string);

//轮询接收接口,非阻塞接收
bool UART_TryReadByte(UART_Port_t port, uint8_t *data);

//硬件信息访问接口
//返回UART寄存器实例
UART_Regs *UART_GetInstance(UART_Port_t port);
//返回中断号
IRQn_Type UART_GetIRQn(UART_Port_t port);
//返回当前配置认为该UART使用的输入时钟
uint32_t UART_GetClockFrequency(UART_Port_t port);

/*
 * 接管已经由 SysConfig 初始化完成的 UART。
 * 不复位、不重新配置外设，只建立 UART.c 内部的软件状态。
 */
bool UART_AttachConfigured(
    UART_Port_t port);

#endif

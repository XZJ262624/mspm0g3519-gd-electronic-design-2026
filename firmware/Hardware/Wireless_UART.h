#ifndef WIRELESS_UART_H
#define WIRELESS_UART_H

#include <stdint.h>

#define WIRELESS_UART_BUFFER_SIZE       (64U)
#define WIRELESS_UART_SEND_CHUNK_SIZE   (30U)
#define WIRELESS_UART_TIMEOUT_COUNT     (100U)

/*
 * 返回值：
 * 0：初始化成功
 * 1：初始化失败
 */
uint8_t wireless_uart_init(void);

/*
 * send_byte返回：
 * 0：发送成功
 * 1：发送失败或RTS超时
 */
uint32_t wireless_uart_send_byte(uint8_t data);

/*
 * send_buffer/send_string返回尚未发送的字节数：
 * 0：全部发送完成
 * 非0：RTS超时或发送失败
 */
uint32_t wireless_uart_send_buffer(
    const uint8_t *buffer,
    uint32_t length);

uint32_t wireless_uart_send_string(
    const char *string);

/*
 * 返回实际读取到的字节数。
 */
uint32_t wireless_uart_read_buffer(
    uint8_t *buffer,
    uint32_t length);

uint32_t wireless_uart_rx_used(void);

#endif
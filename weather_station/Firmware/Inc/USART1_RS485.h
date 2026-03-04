#ifndef __USART1_RS485_H
#define __USART1_RS485_H

#include "main.h"

/* 宏定义 */
#define RS485_DIR_PIN       GPIO_PIN_1
#define RS485_DIR_PORT      GPIOA
#define RS485_TX            GPIO_PIN_2
#define RS485_RX            GPIO_PIN_3
#define RS485_GPIO_Mode     GPIO_MODE_AF_PP
#define RS485_GPIO_AF       GPIO_AF1_USART1

#define Baud_Rate						9600

#define RS485_TX_ENABLE GPIOA->BSRR = RS485_DIR_PIN       // 拉低，使能发送
#define RS485_RX_ENABLE GPIOA->BRR = RS485_DIR_PIN        // 拉高，使能接收

void USART1_Init(void);                                   // 初始化USART1为RS485模式
void USART1_Send_Byte(uint8_t *buffer,uint16_t num);       // 发送字节
uint16_t USART1_Receive(uint8_t *buf, uint16_t maxlen, uint32_t timeout_ms);       // 接收字节
void USART1_Send_String(const char *s);                    // 发送字符串
void USART1_Send_Dec(uint32_t v);                         // 发送十进制数
void USART1_Send_Float2(float v);                         // 发送浮点数


#endif

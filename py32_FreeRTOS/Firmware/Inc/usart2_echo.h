#ifndef __USART2_ECHO_H
#define __USART2_ECHO_H

#include "main.h"
#include <stdio.h>

#define USART2_TX_PIN   GPIO_PIN_4
#define USART2_RX_PIN   GPIO_PIN_5
#define USART2_AF       GPIO_AF9_USART2
#define USART2_DEFAULT_BAUD 9600    // USART2 default baud rate
#define RxBuf_LENGHT2 128           // USART2接收缓冲区长度

extern uint8_t Response_Container[RxBuf_LENGHT2];
extern uint16_t Response_Length;

void USART2_Echo_Init(void);           // 初始化USART2为回显模式
void USART2_Echo_Task(void);                   // 处理USART2回显任务
uint8_t USART2_Periodic_Send_Task(void);          // USART2定时发送任务
void USART2_Send(const uint8_t *buf, uint16_t len);     // 发送数据到USART2
uint16_t USART2_Receive(uint8_t *buf, uint16_t maxlen, uint32_t timeout_ms);     // 接收数据从USART2
uint16_t USART2_ReceiveFrame(uint8_t *buf, uint16_t maxlen);     // 接收数据从USART2，直到收到完整帧
void USART2_ISR(void); // USART2 interrupt service routine callback

#endif

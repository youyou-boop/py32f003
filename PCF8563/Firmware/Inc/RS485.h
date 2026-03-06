#ifndef __RS485_H
#define __RS485_H

#include "main.h"

#define RS485_TX_ENABLE GPIOA->BSRR = GPIO_PIN_1
#define RS485_RX_ENABLE GPIOA->BRR = GPIO_PIN_1

void Modbus_Send_Byte(uint8_t *buffer,uint16_t num);
void USART_Config(void);

uint16_t RS485_Receive(uint8_t *buf, uint16_t maxlen, uint32_t timeout_ms);

#endif

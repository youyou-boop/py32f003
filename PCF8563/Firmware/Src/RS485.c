#include "RS485.h"

UART_HandleTypeDef UartHandle;
uint8_t aTxBuffer[20];

// 接收缓冲区：中断将数据写入 aRxBuffer，空闲中断置位 Frame_Ok 表示一帧结束
#define RxBuf_LENGHT 120				
uint8_t cRxIndex = 0;
uint8_t aRxBuffer[RxBuf_LENGHT];
uint8_t Frame_Ok = 0;

#define Baud_Rate_Default 9600			// 默认波特率9600
/**
 * @brief 发送字节
 * @param buffer 字节数组
 * @param num 字节数
 * @retval None
 */
void Modbus_Send_Byte(uint8_t *buffer,uint16_t num)
{
	RS485_TX_ENABLE;

	HAL_UART_Transmit(&UartHandle, buffer, num,5000);

	RS485_RX_ENABLE;
}

/**
 * @brief 接收字节
 * @param buf 接收缓冲区
 * @param maxlen 最大接收字节数
 * @param timeout_ms 超时时间（毫秒）
 * @retval 实际接收字节数
 */
uint16_t RS485_Receive(uint8_t *buf, uint16_t maxlen, uint32_t timeout_ms)
{
	uint32_t start = HAL_GetTick();
	while (!Frame_Ok)
	{
		if ((HAL_GetTick() - start) >= timeout_ms) return 0;
	}
	Frame_Ok = 0;
	uint16_t n = cRxIndex;
	if (n > maxlen) n = maxlen;
	for (uint16_t i = 0; i < n; i++) buf[i] = aRxBuffer[i];
	cRxIndex = 0;
	return n;
}

int fputc(int ch, FILE *f)
{
  /* Send a byte to USART */
  HAL_UART_Transmit(&UartHandle, (uint8_t *)&ch, 1, 1000);

  return (ch);
}

/**
 * @brief 配置USART1
 * @param None
 * @retval None
 */
void USART_Config(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct = {0};
  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  UartHandle.Instance          = USART1;
  UartHandle.Init.BaudRate     = Baud_Rate_Default;
  UartHandle.Init.WordLength   = UART_WORDLENGTH_8B;
  UartHandle.Init.StopBits     = UART_STOPBITS_1;
  UartHandle.Init.Parity       = UART_PARITY_NONE;
  UartHandle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  UartHandle.Init.Mode         = UART_MODE_TX_RX;
  
  if(HAL_UART_Init(&UartHandle) != HAL_OK)
  {
     Error_Handler();
  } 
	/**USART1 GPIO Configuration
    PA2     ------> USART1_TX
    PA3     ------> USART1_RX
    */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF1_USART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* NVIC */
  HAL_NVIC_SetPriority(USART1_IRQn,0,1);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
	__HAL_UART_ENABLE_IT(&UartHandle, UART_IT_RXNE);
	__HAL_UART_ENABLE_IT(&UartHandle, UART_IT_IDLE);
	
	RS485_RX_ENABLE;
	
	GPIO_InitStruct.Pin = GPIO_PIN_1;//dir
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* 串口中断配置 */
void USART1_IRQHandler(void)
{
	uint32_t isrflags = READ_REG((&UartHandle)->Instance->SR);
	if(isrflags & UART_FLAG_RXNE)
	{
		aRxBuffer[cRxIndex] = (uint8_t)((&UartHandle)->Instance->DR & 0xFF);
		if(cRxIndex < RxBuf_LENGHT-1) cRxIndex++;
	}
	if(isrflags & UART_FLAG_IDLE)
	{
		volatile uint32_t tmp;
		tmp = (&UartHandle)->Instance->SR; (void)tmp;
		tmp = (&UartHandle)->Instance->DR; (void)tmp;
		Frame_Ok = 1;
	}
}

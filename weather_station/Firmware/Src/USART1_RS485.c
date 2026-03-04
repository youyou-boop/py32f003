#include "USART1_RS485.h"
#include "modbus.h"
#include <string.h>

UART_HandleTypeDef UartHandle;

// 接收缓冲区：中断将数据写入 aRxBuffer，空闲中断置位 Frame_Ok 表示一帧结束
#define RxBuf_LENGHT 120				      // 接收缓冲区长度
uint8_t cRxIndex = 0;                 // 接收索引
uint8_t aRxBuffer[RxBuf_LENGHT];      // 接收缓冲区
volatile uint8_t Frame_Ok = 0;        // 接收完成标志

/* 初始化USART1 */
void USART1_Config(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct = {0};
  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  UartHandle.Instance          = USART1;
  UartHandle.Init.BaudRate     = modbus_get_saved_baudrate();   // 从Flash保存的代码换算实际波特率（断电后生效）
  UartHandle.Init.WordLength   = UART_WORDLENGTH_8B;
  UartHandle.Init.StopBits     = UART_STOPBITS_1;
  UartHandle.Init.Parity       = UART_PARITY_NONE;
  UartHandle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  UartHandle.Init.Mode         = UART_MODE_TX_RX;
  
  if(HAL_UART_Init(&UartHandle) != HAL_OK)    // 判断初始化是否成功
  {
     Error_Handler();
  } 

  GPIO_InitStruct.Pin = RS485_TX | RS485_RX;
  GPIO_InitStruct.Mode = RS485_GPIO_Mode;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = RS485_GPIO_AF;
  HAL_GPIO_Init(RS485_DIR_PORT, &GPIO_InitStruct);

	/* NVIC */
  HAL_NVIC_SetPriority(USART1_IRQn,0,1);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
	__HAL_UART_ENABLE_IT(&UartHandle, UART_IT_RXNE);
	__HAL_UART_ENABLE_IT(&UartHandle, UART_IT_IDLE);
	
  GPIO_InitStruct.Pin = RS485_DIR_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  HAL_GPIO_Init(RS485_DIR_PORT, &GPIO_InitStruct);
	
	RS485_RX_ENABLE;
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
/* 发送字节 */
void USART1_Send_Byte(uint8_t *buffer,uint16_t num){
    RS485_TX_ENABLE;        // 使能发送
    HAL_UART_Transmit(&UartHandle, buffer, num, 100);
    while(__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_TC) == RESET); // 等待发送完成
    RS485_RX_ENABLE;        // 使能接收
}

/* 发送字符串 */
void USART1_Send_String(const char *s){
    USART1_Send_Byte((uint8_t*)s, (uint16_t)strlen(s));
}

/* 发送32位无符号整数 */
static uint16_t u32_to_dec(uint32_t v, char *buf){
    if(v == 0){ buf[0]='0'; return 1; }
    char tmp[10];
    uint16_t i=0;
    while(v>0){ tmp[i++] = (char)('0' + (v % 10)); v/=10; }
    for(uint16_t j=0;j<i;j++) buf[j] = tmp[i-1-j];
    return i;
}

/* 发送32位无符号整数 */
void USART1_Send_Dec(uint32_t v){
    char b[10];
    uint16_t n = u32_to_dec(v, b);
    USART1_Send_Byte((uint8_t*)b, n);
}

/* 发送浮点数（保留2位小数） */
void USART1_Send_Float2(float v){
    if (v < 0) {
        USART1_Send_String("-");
        v = -v;
    }
    uint32_t ip = (uint32_t)v;
    uint32_t fp = (uint32_t)((v - (float)ip) * 100.0f + 0.5f);
    USART1_Send_Dec(ip);
    USART1_Send_String(".");
    if (fp < 10) USART1_Send_String("0");
    USART1_Send_Dec(fp);
}

/**
 * @brief 接收字节
 * @param buf 接收缓冲区
 * @param maxlen 最大接收字节数
 * @param timeout_ms 超时时间（毫秒）
 * @retval 实际接收字节数
 */
uint16_t USART1_Receive(uint8_t *buf, uint16_t maxlen, uint32_t timeout_ms)
{
	uint32_t start = HAL_GetTick();
	while (!Frame_Ok)
	{
		if ((HAL_GetTick() - start) >= timeout_ms) {
            return 0;
        }
        if (timeout_ms == 0) {
            return 0;
        }
	}
	Frame_Ok = 0;
	uint16_t n = cRxIndex;
	if (n > maxlen) n = maxlen;
	for (uint16_t i = 0; i < n; i++) buf[i] = aRxBuffer[i];
	cRxIndex = 0;
	return n;
}

/* 主函数初始化 */
void USART1_Init(void){
    USART1_Config();
}


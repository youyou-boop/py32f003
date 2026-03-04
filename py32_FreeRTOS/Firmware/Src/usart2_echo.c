#include "usart2_echo.h"

static UART_HandleTypeDef Uart2Handle;     // USART2 句柄，仅在本文件使用

// 接收缓冲区：中断将数据写入 aRxBuffer2，空闲中断置位 Frame_Ok2 表示一帧结束
// #define RxBuf_LENGHT2 120 // 已在头文件中定义
volatile uint8_t  cRxIndex2 = 0;           // 接收计数（ISR 修改，任务读取）
uint8_t           aRxBuffer2[RxBuf_LENGHT2];
volatile uint8_t  Frame_Ok2 = 0;           // 空闲帧完成标志（由 IDLE 中断置位）
static volatile uint32_t last_rx_tick2 = 0; // 最近一次收到字节的时间戳（用于无 IDLE 的超时组帧）

void USART2_Send(const uint8_t *buf, uint16_t len)
{
    HAL_UART_Transmit(&Uart2Handle, (uint8_t *)buf, len, 1000);
}

uint16_t USART2_ReceiveFrame(uint8_t *buf, uint16_t maxlen)
{
    if (!Frame_Ok2) return 0;
    Frame_Ok2 = 0;
    uint16_t n = cRxIndex2;
    if (n > maxlen) n = maxlen;
    for (uint16_t i = 0; i < n; i++) buf[i] = aRxBuffer2[i];
    cRxIndex2 = 0;
    return n;
}

/**
 * @brief 接收USART2数据
 * 
 * 该函数用于从USART2接收数据，最多接收maxlen个字节，超时时间为timeout_ms毫秒。
 * 
 * @param buf 接收数据的缓冲区指针
 * @param maxlen 最大接收字节数
 * @param timeout_ms 超时时间（毫秒）
 * @return uint16_t 实际接收的字节数
 */
uint16_t USART2_Receive(uint8_t *buf, uint16_t maxlen, uint32_t timeout_ms)
{
    uint16_t n = 0;
    uint32_t last = HAL_GetTick();
    while (n < maxlen)
    {
        if (__HAL_UART_GET_FLAG(&Uart2Handle, UART_FLAG_RXNE))
        {
            buf[n++] = (uint8_t)(Uart2Handle.Instance->DR & 0xFF);
            last = HAL_GetTick();
        }
        else
        {
            if ((HAL_GetTick() - last) >= timeout_ms) break;
        }
    }
    return n;
}

static void GPIO_Config(void){
	
	  __HAL_RCC_GPIOA_CLK_ENABLE();       // 使能GPIOA时钟
	  GPIO_InitTypeDef GPIO_InitStruct;
	
	  GPIO_InitStruct.Pin = USART2_TX_PIN | USART2_RX_PIN; 
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = USART2_AF;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void USART2_Config(void){
    __HAL_RCC_USART2_CLK_ENABLE();      // 使能USART2时钟

    Uart2Handle.Instance = USART2;
    Uart2Handle.Init.BaudRate = USART2_DEFAULT_BAUD;	// baud --> 9600
    Uart2Handle.Init.WordLength = UART_WORDLENGTH_8B;
    Uart2Handle.Init.StopBits = UART_STOPBITS_1;
    Uart2Handle.Init.Parity = UART_PARITY_NONE;
    Uart2Handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    Uart2Handle.Init.Mode = UART_MODE_TX_RX;
    if (HAL_UART_Init(&Uart2Handle) != HAL_OK) Error_Handler();
	
    HAL_NVIC_SetPriority(USART2_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    __HAL_UART_ENABLE_IT(&Uart2Handle, UART_IT_RXNE);
		__HAL_UART_ENABLE_IT(&Uart2Handle, UART_IT_IDLE);
}

void USART2_Echo_Init()
{
	GPIO_Config();
	USART2_Config();
}

/**
 * @brief 处理USART2接收事件
 * 
 * 当USART2接收到一帧数据时，将数据回显到USART2。
 * 
 * @note 该函数在主循环中调用，用于处理USART2接收事件。
 */
void USART2_Echo_Task(void)
{
    // 情况1：收到 IDLE 中断判定为一帧完成
    if (Frame_Ok2)
    {
        Frame_Ok2 = 0;
        if (cRxIndex2 > 0)
        {
            USART2_Send(aRxBuffer2, cRxIndex2);
            cRxIndex2 = 0;
        }
    }
    // 情况2：未开启/未触发 IDLE，但已有数据并超过超时阈值，认为一帧结束
    else if (cRxIndex2 > 0)
    {
        if ((HAL_GetTick() - last_rx_tick2) >= 10) // 约10ms超时：在9600bps下足够判帧
        {
            USART2_Send(aRxBuffer2, cRxIndex2);
            cRxIndex2 = 0;
        }
    }
}

/**
 * @brief USART2中断服务程序
 * 
 * 当USART2接收到数据或发生空闲中断时，会触发该中断。
 * 该函数用于处理USART2的接收事件和空闲中断。
 * 
 * @note 该函数在中断服务程序中调用，用于处理USART2的接收事件和空闲中断。
 */
void USART2_ISR(void) 
{
  uint32_t isrflags = READ_REG((&Uart2Handle)->Instance->SR);
	if(isrflags & UART_FLAG_RXNE)
	{
		aRxBuffer2[cRxIndex2] = (uint8_t)((&Uart2Handle)->Instance->DR & 0xFF);
		if(cRxIndex2 < RxBuf_LENGHT2-1) {       // 未超缓冲区
      cRxIndex2++;
    } else {
      Frame_Ok2 = 1;
    }
    last_rx_tick2 = HAL_GetTick();         // 记录最近一次收字节时间
	}
	if(isrflags & UART_FLAG_IDLE)
	{
		volatile uint32_t tmp;
		tmp = (&Uart2Handle)->Instance->SR; (void)tmp;
		tmp = (&Uart2Handle)->Instance->DR; (void)tmp;
		Frame_Ok2 = 1;
	}
}

#include "usart2_echo.h"
#include "modbus.h"

static UART_HandleTypeDef Uart2Handle;

// 接收缓冲区：中断将数据写入 aRxBuffer2，空闲中断置位 Frame_Ok2 表示一帧结束
// #define RxBuf_LENGHT2 120 // Moved to header
uint8_t cRxIndex2 = 0;
uint8_t aRxBuffer2[RxBuf_LENGHT2];
volatile uint8_t Frame_Ok2 = 0;

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

void GPIO_Config(void){
	
	  __HAL_RCC_GPIOA_CLK_ENABLE();       // 使能GPIOA时钟
	  GPIO_InitTypeDef GPIO_InitStruct;
	
	  GPIO_InitStruct.Pin = USART2_TX_PIN | USART2_RX_PIN; 
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = USART2_AF;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void USART2_Config(void){
    __HAL_RCC_USART2_CLK_ENABLE();      // 使能USART2时钟

    Uart2Handle.Instance = USART2;
    Uart2Handle.Init.BaudRate = USART2_DEFAULT_BAUD;	// baud --> 9600
    Uart2Handle.Init.WordLength = UART_WORDLENGTH_8B;
    Uart2Handle.Init.StopBits = UART_STOPBITS_1;
    Uart2Handle.Init.Parity = UART_PARITY_NONE;
    Uart2Handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    Uart2Handle.Init.Mode = UART_MODE_TX_RX;
    if (HAL_UART_Init(&Uart2Handle) != HAL_OK) Error_Handler();
	
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
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
    if (Frame_Ok2)
    {
        Frame_Ok2 = 0;
        if (cRxIndex2 > 0)
        {
            USART2_Send(aRxBuffer2, cRxIndex2);
            cRxIndex2 = 0;
        }
    }
}

// 定义特征码
const uint8_t FEATURE_CODE[] = {0xFE, 0x04, 0x00, 0x03, 0x00, 0x01, 0xD5, 0xC5};
// 定义接收容器
uint8_t Response_Container[RxBuf_LENGHT2];
uint16_t Response_Length = 0;
uint32_t Last_Send_Time = 0;

/**
 * @brief USART2定时发送任务
 * 
 * 每10ms发送一次特征码，并将接收到的数据存储在Response_Container中。
 * 
 * @note 该函数需要在主循环中调用。
 * @return uint8_t 1: 接收到新数据, 0: 未接收到新数据
 */
uint8_t USART2_Periodic_Send_Task(void)
{
    uint8_t received = 0;
    // 检查是否达到10ms发送间隔
    if (HAL_GetTick() - Last_Send_Time >= 100)
    {
        Last_Send_Time = HAL_GetTick();
        // 发送特征码
        USART2_Send(FEATURE_CODE, sizeof(FEATURE_CODE));
    }

    // 检查是否有数据接收
    if (Frame_Ok2)
    {
        Frame_Ok2 = 0; // 清除帧接收标志
        
        // 确保接收到的数据长度不超过容器大小
        uint16_t len = (cRxIndex2 > RxBuf_LENGHT2) ? RxBuf_LENGHT2 : cRxIndex2;
        
        // 将接收到的数据复制到容器中
        for (uint16_t i = 0; i < len; i++)
        {
            Response_Container[i] = aRxBuffer2[i];
        }
        Response_Length = len; // 更新接收数据长度
        
        cRxIndex2 = 0; // 重置接收缓冲区索引
        received = 1;
    }
    return received;
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
		if(cRxIndex2 < RxBuf_LENGHT2-1) cRxIndex2++;
	}
	if(isrflags & UART_FLAG_IDLE)
	{
		volatile uint32_t tmp;
		tmp = (&Uart2Handle)->Instance->SR; (void)tmp;
		tmp = (&Uart2Handle)->Instance->DR; (void)tmp;
		Frame_Ok2 = 1;
	}
}

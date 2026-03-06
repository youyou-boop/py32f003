#include "i2c.h"
#include "py32f0xx_hal_i2c.h"

#if USE_HARDWARE_I2C

I2C_HandleTypeDef hi2c1;

/* I2C1 Bus Reset / Unlock Function */
void I2C_ResetBus(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  /* 1. Disable I2C Peripheral */
  __HAL_RCC_I2C_CLK_DISABLE();
  
  /* 2. Configure SCL & SDA as Output Open-Drain */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  /* 3. Toggle SCL 9 times to release SDA */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET); // SDA High
  for (int i = 0; i < 9; i++)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); // SCL Low
    HAL_Delay(1); // Ensure enough time
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);   // SCL High
    HAL_Delay(1);
  }
  
  /* 4. Generate STOP condition manually */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
  HAL_Delay(1);
  
  /* 5. Re-enable I2C Clock (will be done in HAL_I2C_Init) */
}

/* I2C1 init function */
void I2C_Init(void)
{
  /* 0. 先对总线进行手动复位，解决死锁 */
  I2C_ResetBus();

  /* 1. 强制复位 I2C 外设（清除内部状态机错误） */
  __HAL_RCC_I2C_CLK_ENABLE(); // 必须先开时钟才能复位
  __HAL_RCC_I2C_FORCE_RESET();
  HAL_Delay(2); // 保持复位状态一小段时间
  __HAL_RCC_I2C_RELEASE_RESET();
  
  /* 2. 配置 I2C1 参数 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 10000; // 再次降到 10kHz，以确保时序稳定
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  /* PY32F0xx HAL 库简化版 I2C_InitTypeDef，无 AddressingMode 等成员 */
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  
  /* 3. 初始化 I2C 外设 
   * 注意：HAL_I2C_Init 会自动调用 HAL_I2C_MspInit()
   * 该函数在 py32f0xx_hal_msp.c 中定义，负责 GPIO 和 时钟的初始化
   */
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    /* Initialization Error */
    while(1);
  }
}

/* 封装：探测设备是否存在 */
HAL_StatusTypeDef I2C_Probe(uint8_t addr, uint32_t timeout_ms)
{
  /* HAL_I2C_IsDeviceReady 的地址需要左移1位 */
  HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr << 1), 2, timeout_ms);
  if (status != HAL_OK)
  {
      // 探测失败时尝试复位总线，防止后续操作全部挂死
      // I2C_ResetBus(); // 暂不在此处强制复位，以免过于频繁
      // HAL_I2C_Init(&hi2c1);
  }
  return status;
}

/* 封装：等待设备就绪 */
HAL_StatusTypeDef I2C_IsReady(uint8_t addr, uint32_t trials, uint32_t timeout_ms)
{
  return HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr << 1), trials, timeout_ms);
}

/* 封装：写寄存器 */
HAL_StatusTypeDef I2C_WriteReg(uint8_t devAddr, uint8_t reg, const uint8_t *data, uint16_t size, uint32_t timeout_ms)
{
  /* 使用 HAL_I2C_Mem_Write，注意 MemAddSize 为 I2C_MEMADD_SIZE_8BIT */
  return HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(devAddr << 1), (uint16_t)reg, I2C_MEMADD_SIZE_8BIT, (uint8_t*)data, size, timeout_ms);
}

/* 封装：读寄存器 (改为拆分传输模式，避免 Repeated Start 兼容性问题) */
HAL_StatusTypeDef I2C_ReadReg(uint8_t devAddr, uint8_t reg, uint8_t *data, uint16_t size, uint32_t timeout_ms)
{
  HAL_StatusTypeDef status;
  
  /* 1. 发送寄存器地址 (Write Address + Register) */
  /* 注意：这里不使用 Mem_Read，而是先 Master_Transmit 再 Master_Receive */
  /* 发送寄存器地址时，不需要发送数据，只发送寄存器指针 */
  status = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(devAddr << 1), &reg, 1, timeout_ms);
  if (status != HAL_OK)
  {
      // 如果写地址失败，尝试复位 I2C 外设以清除错误状态
      if (status == HAL_ERROR) {
          __HAL_I2C_CLEAR_FLAG(&hi2c1, I2C_FLAG_AF); // 清除 ACK 失败标志
          /* 甚至可以考虑 I2C_Init(); 但这里先只清标志 */
      }
      return status;
  }
  
  /* 2. 读取数据 (Read Address + Data) */
  /* 中间会产生 STOP + START，这对 PCF8563 是合法的，且比 Repeated Start 更稳定 */
  return HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(devAddr << 1), data, size, timeout_ms);
}

#else
/* -------------------------------------------------------------------------- */
/*                               软件模拟 I2C 实现                               */
/* -------------------------------------------------------------------------- */

#define I2C_SCL_PORT GPIOB
#define I2C_SDA_PORT GPIOB
#define I2C_SCL_PIN GPIO_PIN_6
#define I2C_SDA_PIN GPIO_PIN_7
#define I2C_DELAY_TICKS 5000U

static void I2C_Delay(void)
{
  for (volatile uint32_t i = 0; i < I2C_DELAY_TICKS; i++)
  {
    __NOP();
  }
}

static void I2C_SCL_High(void)
{
  HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
}

static void I2C_SCL_Low(void)
{
  HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
}

static void I2C_SDA_High(void)
{
  HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
}

static void I2C_SDA_Low(void)
{
  HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);
}

static uint8_t I2C_ReadSDA(void)
{
  return (HAL_GPIO_ReadPin(I2C_SDA_PORT, I2C_SDA_PIN) == GPIO_PIN_SET) ? 1U : 0U;
}

static void I2C_Start(void)
{
  I2C_SDA_High();
  I2C_SCL_High();
  I2C_Delay();
  I2C_SDA_Low();
  I2C_Delay();
  I2C_SCL_Low();
  I2C_Delay();
}

static void I2C_Stop(void)
{
  I2C_SCL_Low();
  I2C_SDA_Low();
  I2C_Delay();
  I2C_SCL_High();
  I2C_Delay();
  I2C_SDA_High();
  I2C_Delay();
}

static void I2C_WriteByte(uint8_t byte)
{
  for (uint8_t i = 0; i < 8U; i++)
  {
    I2C_SCL_Low();
    I2C_Delay();
    if (byte & 0x80U)
    {
      I2C_SDA_High();
    }
    else
    {
      I2C_SDA_Low();
    }
    I2C_Delay();
    I2C_SCL_High();
    I2C_Delay();
    I2C_SCL_Low();
    I2C_Delay();
    byte <<= 1U;
  }
  I2C_SDA_High();
  I2C_Delay();
}

static HAL_StatusTypeDef I2C_WaitAck(uint32_t timeout_ms)
{
  I2C_SCL_Low();
  I2C_Delay();
  I2C_SDA_High();
  I2C_Delay();
  I2C_SCL_High();
  I2C_Delay();
  
  if (I2C_ReadSDA())
  {
    I2C_SCL_Low();
    return HAL_ERROR;
  }
  
  I2C_SCL_Low();
  I2C_Delay();
  return HAL_OK;
}

static void I2C_SendAck(uint8_t nack)
{
  I2C_SCL_Low();
  I2C_Delay();
  if (nack)
  {
    I2C_SDA_High();
  }
  else
  {
    I2C_SDA_Low();
  }
  I2C_Delay();
  I2C_SCL_High();
  I2C_Delay();
  I2C_SCL_Low();
  I2C_Delay();
  I2C_SDA_High();
  I2C_Delay();
}

static uint8_t I2C_ReadByte(uint8_t nack)
{
  uint8_t byte = 0;
  I2C_SDA_High();
  for (uint8_t i = 0; i < 8U; i++)
  {
    I2C_SCL_Low();
    I2C_Delay();
    I2C_SCL_High();
    I2C_Delay();
    byte <<= 1U;
    if (I2C_ReadSDA())
    {
      byte |= 1U;
    }
    I2C_SCL_Low();
    I2C_Delay();
  }
  I2C_SendAck(nack);
  return byte;
}

void I2C_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOB_CLK_ENABLE();
  
  GPIO_InitStruct.Pin = I2C_SCL_PIN | I2C_SDA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; // 开漏输出
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  I2C_SCL_High();
  I2C_SDA_High();
  
  // 发送 9 个时钟脉冲，确保从机释放 SDA
  for (int i = 0; i < 9; i++)
  {
    I2C_SCL_Low();
    I2C_Delay();
    I2C_SCL_High();
    I2C_Delay();
  }
  I2C_Stop();
}

HAL_StatusTypeDef I2C_Probe(uint8_t addr, uint32_t timeout_ms)
{
  uint8_t address = (uint8_t)(addr << 1U);
  I2C_Start();
  I2C_WriteByte(address);
  HAL_StatusTypeDef ret = I2C_WaitAck(timeout_ms);
  I2C_Stop();
  return ret;
}

HAL_StatusTypeDef I2C_IsReady(uint8_t addr, uint32_t trials, uint32_t timeout_ms)
{
  if (trials == 0U)
  {
    trials = 1U;
  }
  uint32_t start = HAL_GetTick();
  for (uint32_t i = 0; i < trials; i++)
  {
    HAL_StatusTypeDef ret = I2C_Probe(addr, timeout_ms);
    if (ret == HAL_OK)
    {
      return HAL_OK;
    }
    if (timeout_ms != 0U && (HAL_GetTick() - start) >= timeout_ms)
    {
      return HAL_TIMEOUT;
    }
    HAL_Delay(1);
  }
  return HAL_ERROR;
}

HAL_StatusTypeDef I2C_WriteReg(uint8_t devAddr, uint8_t reg, const uint8_t *data, uint16_t size, uint32_t timeout_ms)
{
  if (data == NULL || size == 0U)
  {
    return HAL_ERROR;
  }
  uint8_t address = (uint8_t)(devAddr << 1U);
  I2C_Start();
  I2C_WriteByte(address);
  if (I2C_WaitAck(timeout_ms) != HAL_OK)
  {
    I2C_Stop();
    return HAL_ERROR;
  }
  I2C_WriteByte(reg);
  if (I2C_WaitAck(timeout_ms) != HAL_OK)
  {
    I2C_Stop();
    return HAL_ERROR;
  }
  for (uint16_t i = 0; i < size; i++)
  {
    I2C_WriteByte(data[i]);
    if (I2C_WaitAck(timeout_ms) != HAL_OK)
    {
      I2C_Stop();
      return HAL_ERROR;
    }
  }
  I2C_Stop();
  return HAL_OK;
}

HAL_StatusTypeDef I2C_ReadReg(uint8_t devAddr, uint8_t reg, uint8_t *data, uint16_t size, uint32_t timeout_ms)
{
  if (data == NULL || size == 0U)
  {
    return HAL_ERROR;
  }
  uint8_t address_write = (uint8_t)(devAddr << 1U);
  uint8_t address_read = (uint8_t)((devAddr << 1U) | 0x01U);
  I2C_Start();
  I2C_WriteByte(address_write);
  if (I2C_WaitAck(timeout_ms) != HAL_OK)
  {
    I2C_Stop();
    return HAL_ERROR;
  }
  I2C_WriteByte(reg);
  if (I2C_WaitAck(timeout_ms) != HAL_OK)
  {
    I2C_Stop();
    return HAL_ERROR;
  }
  I2C_Start();
  I2C_WriteByte(address_read);
  if (I2C_WaitAck(timeout_ms) != HAL_OK)
  {
    I2C_Stop();
    return HAL_ERROR;
  }
  for (uint16_t i = 0; i < size; i++)
  {
    uint8_t nack = (i == (uint16_t)(size - 1U)) ? 1U : 0U;
    data[i] = I2C_ReadByte(nack);
  }
  I2C_Stop();
  return HAL_OK;
}

#endif

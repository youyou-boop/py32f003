#ifndef __I2C_H
#define __I2C_H

#include "main.h"

// 定义使用硬件 I2C 还是软件模拟 I2C
#define USE_HARDWARE_I2C  1

#if USE_HARDWARE_I2C
extern I2C_HandleTypeDef hi2c1;
#endif

void I2C_Init(void);
/* @brief 扫描 I2C 总线上的设备
 * @param addr: 要扫描的设备地址
 * @param timeout_ms: 超时时间（毫秒）
 * @return HAL_StatusTypeDef: HAL_OK 表示成功，HAL_TIMEOUT 表示超时
 */
HAL_StatusTypeDef I2C_Probe(uint8_t addr, uint32_t timeout_ms);
/** @brief 检查 I2C 设备是否准备好
 * @param addr: 要检查的设备地址
 * @param trials: 重试次数
 * @param timeout_ms: 每次重试的超时时间（毫秒）
 * @return HAL_StatusTypeDef: HAL_OK 表示成功，HAL_TIMEOUT 表示超时
 */
HAL_StatusTypeDef I2C_IsReady(uint8_t addr, uint32_t trials, uint32_t timeout_ms);
HAL_StatusTypeDef I2C_ReadReg(uint8_t devAddr, uint8_t reg, uint8_t *data, uint16_t size, uint32_t timeout_ms);
HAL_StatusTypeDef I2C_WriteReg(uint8_t devAddr, uint8_t reg, const uint8_t *data, uint16_t size, uint32_t timeout_ms);

#endif

#ifndef I2C_FOR_GDKG_H
#define I2C_FOR_GDKG_H

#include "main.h"
#include "py32f0xx_hal.h"
#include <stdbool.h>

// I2C配置
#define I2C_FOR_GDKG_CLOCK_SPEED   100000      // 100kHz标准模式
#define I2C_FOR_GDKG_TIMEOUT       1000        // 传输超时(ms)
#define I2C_FOR_GDKG_ADDRESS       0x40        // GDKG传感器的I2C地址

// GDKG传感器I2C寄存器映射
#define GDKG_I2C_REG_STATUS         0x00        // 状态寄存器
#define GDKG_I2C_REG_VELOCITY_MPS   0x01        // 速度(m/s)寄存器(2字节)
#define GDKG_I2C_REG_VELOCITY_KMPH  0x03        // 速度(km/h)寄存器(2字节)
#define GDKG_I2C_REG_PULSE_COUNT    0x05        // 脉冲计数寄存器(4字节)
#define GDKG_I2C_REG_DEVICE_ID      0x09        // 设备ID寄存器
#define GDKG_I2C_REG_CTRL           0x0A        // 控制寄存器

// 状态寄存器位定义
#define GDKG_STATUS_READY           (1 << 0)    // 数据就绪
#define GDKG_STATUS_MEASURING       (1 << 1)    // 正在测量
#define GDKG_STATUS_ERROR           (1 << 2)    // 错误状态

// 控制寄存器位定义
#define GDKG_CTRL_RESET_PULSE       (1 << 0)    // 重置脉冲计数
#define GDKG_CTRL_START_MEASURE     (1 << 1)    // 开始测量
#define GDKG_CTRL_STOP_MEASURE      (1 << 2)    // 停止测量

// I2C句柄声明
extern I2C_HandleTypeDef hi2c_for_gdkg;

// 初始化函数
HAL_StatusTypeDef I2C_For_GDKG_Init(void);
bool I2C_For_GDKG_IsInitialized(void);

// 设备检测函数
HAL_StatusTypeDef I2C_For_GDKG_CheckDevice(void);
HAL_StatusTypeDef I2C_For_GDKG_ScanBus(uint8_t* found_devices, uint8_t* count);

// 读取速度数据
HAL_StatusTypeDef I2C_For_GDKG_ReadVelocity(float* velocity_mps, float* velocity_kmph);
HAL_StatusTypeDef I2C_For_GDKG_ReadVelocityRaw(uint8_t* buffer, uint8_t size);
HAL_StatusTypeDef I2C_For_GDKG_ReadPulseCount(uint32_t* pulse_count);

// 读取状态
HAL_StatusTypeDef I2C_For_GDKG_ReadStatus(uint8_t* status);
HAL_StatusTypeDef I2C_For_GDKG_ReadDeviceID(uint8_t* device_id);

// 控制函数
HAL_StatusTypeDef I2C_For_GDKG_ResetPulseCount(void);
HAL_StatusTypeDef I2C_For_GDKG_StartMeasurement(void);
HAL_StatusTypeDef I2C_For_GDKG_StopMeasurement(void);

// 工具函数
HAL_StatusTypeDef I2C_For_GDKG_WriteRegister(uint8_t reg_addr, uint8_t value);
HAL_StatusTypeDef I2C_For_GDKG_ReadRegister(uint8_t reg_addr, uint8_t* value);
HAL_StatusTypeDef I2C_For_GDKG_ReadRegisters(uint8_t start_addr, uint8_t* buffer, uint8_t count);

// 获取错误计数
uint32_t I2C_For_GDKG_GetErrorCount(void);

#endif /* I2C_FOR_GDKG_H */

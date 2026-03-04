#include "i2c_for_gdkg.h"
#include "gdkg.h"
#include "py32f0xx_hal.h"
#include "py32f0xx_hal_i2c.h"
#include <string.h>

// 全局I2C句柄
I2C_HandleTypeDef hi2c_for_gdkg = {0};

// 模块状态变量
static bool s_i2c_initialized = false;
static uint32_t s_error_count = 0;

/**
 * @brief 初始化GDKG的I2C接口 (PY32F0系列专用)
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef I2C_For_GDKG_Init(void)
{
    if (s_i2c_initialized) {
        return HAL_OK;
    }
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 使能I2C时钟
    __HAL_RCC_I2C_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    // 配置PB6 (I2C_SCL) 和 PB7 (I2C_SDA) - PY32F0系列
    // 根据PY32F0手册，I2C的复用功能是AF1
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;        // 开漏输出
    GPIO_InitStruct.Pull = GPIO_PULLUP;            // 上拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  // 高速
    
    // PY32F0的I2C复用功能是AF1
    #if defined(GPIO_AF1_I2C)
    GPIO_InitStruct.Alternate = GPIO_AF1_I2C;      // PY32F0的I2C复用功能AF1
    #else
    // 如果没有定义，使用数值1
    GPIO_InitStruct.Alternate = 1;                 // AF1
    #endif
    
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // 复位I2C
    __HAL_RCC_I2C_FORCE_RESET();
    HAL_Delay(1);
    __HAL_RCC_I2C_RELEASE_RESET();
    HAL_Delay(1);
    
    // 配置I2C - PY32F0系列使用简化的初始化结构
    hi2c_for_gdkg.Instance = I2C;
    hi2c_for_gdkg.Init.ClockSpeed = 100000;             // 100kHz
    hi2c_for_gdkg.Init.DutyCycle = I2C_DUTYCYCLE_2;     // 占空比2:1
    hi2c_for_gdkg.Init.OwnAddress1 = 0;                 // 主机模式，不设从机地址
    hi2c_for_gdkg.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;  // 禁用广播呼叫
    hi2c_for_gdkg.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;      // 不禁用时钟延长
    
    
    HAL_StatusTypeDef status = HAL_I2C_Init(&hi2c_for_gdkg);
    if (status != HAL_OK) {
        s_error_count++;
        return status;
    }
    
    s_i2c_initialized = true;
    return HAL_OK;
}

/**
 * @brief 检查I2C是否初始化
 * @retval true 已初始化
 */
bool I2C_For_GDKG_IsInitialized(void)
{
    return s_i2c_initialized;
}

/**
 * @brief 检查GDKG设备是否存在
 * @retval HAL_OK 设备存在
 * @retval HAL_ERROR 设备不存在
 */
HAL_StatusTypeDef I2C_For_GDKG_CheckDevice(void)
{
    if (!s_i2c_initialized) {
        if (I2C_For_GDKG_Init() != HAL_OK) {
            return HAL_ERROR;
        }
    }
    
    return HAL_I2C_IsDeviceReady(&hi2c_for_gdkg, I2C_FOR_GDKG_ADDRESS << 1, 3, 100);
}

/**
 * @brief 扫描I2C总线
 * @param found_devices 找到的设备地址数组
 * @param count 找到的设备数量
 * @retval HAL_OK 成功
 */
HAL_StatusTypeDef I2C_For_GDKG_ScanBus(uint8_t* found_devices, uint8_t* count)
{
    if (!s_i2c_initialized) {
        if (I2C_For_GDKG_Init() != HAL_OK) {
            return HAL_ERROR;
        }
    }
    
    uint8_t found = 0;
    
    // 扫描所有可能的7位地址
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c_for_gdkg, addr << 1, 1, 10) == HAL_OK) {
            if (found_devices != NULL && found < 16) {
                found_devices[found] = addr;
            }
            found++;
        }
    }
    
    if (count != NULL) {
        *count = found;
    }
    
    return HAL_OK;
}

/**
 * @brief 写入寄存器
 * @param reg_addr 寄存器地址
 * @param value 要写入的值
 * @retval HAL_OK 成功
 */
HAL_StatusTypeDef I2C_For_GDKG_WriteRegister(uint8_t reg_addr, uint8_t value)
{
    if (!s_i2c_initialized) {
        if (I2C_For_GDKG_Init() != HAL_OK) {
            return HAL_ERROR;
        }
    }
    
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c_for_gdkg, I2C_FOR_GDKG_ADDRESS << 1,
                                                 reg_addr, I2C_MEMADD_SIZE_8BIT,
                                                 &value, 1, I2C_FOR_GDKG_TIMEOUT);
    
    if (status != HAL_OK) {
        s_error_count++;
    }
    
    return status;
}

/**
 * @brief 读取寄存器
 * @param reg_addr 寄存器地址
 * @param value 返回值指针
 * @retval HAL_OK 成功
 */
HAL_StatusTypeDef I2C_For_GDKG_ReadRegister(uint8_t reg_addr, uint8_t* value)
{
    if (!s_i2c_initialized) {
        if (I2C_For_GDKG_Init() != HAL_OK) {
            return HAL_ERROR;
        }
    }
    
    if (value == NULL) {
        return HAL_ERROR;
    }
    
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c_for_gdkg, I2C_FOR_GDKG_ADDRESS << 1,
                                                reg_addr, I2C_MEMADD_SIZE_8BIT,
                                                value, 1, I2C_FOR_GDKG_TIMEOUT);
    
    if (status != HAL_OK) {
        s_error_count++;
    }
    
    return status;
}

/**
 * @brief 读取多个寄存器
 * @param start_addr 起始地址
 * @param buffer 缓冲区
 * @param count 数量
 * @retval HAL_OK 成功
 */
HAL_StatusTypeDef I2C_For_GDKG_ReadRegisters(uint8_t start_addr, uint8_t* buffer, uint8_t count)
{
    if (!s_i2c_initialized) {
        if (I2C_For_GDKG_Init() != HAL_OK) {
            return HAL_ERROR;
        }
    }
    
    if (buffer == NULL || count == 0) {
        return HAL_ERROR;
    }
    
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c_for_gdkg, I2C_FOR_GDKG_ADDRESS << 1,
                                                start_addr, I2C_MEMADD_SIZE_8BIT,
                                                buffer, count, I2C_FOR_GDKG_TIMEOUT);
    
    if (status != HAL_OK) {
        s_error_count++;
    }
    
    return status;
}

/**
 * @brief 读取速度数据
 * @param velocity_mps 速度(m/s)输出指针
 * @param velocity_kmph 速度(km/h)输出指针
 * @retval HAL_OK 成功
 */
HAL_StatusTypeDef I2C_For_GDKG_ReadVelocity(float* velocity_mps, float* velocity_kmph)
{
    uint8_t buffer[4]; // 2字节m/s + 2字节km/h
    
    // 读取速度数据
    HAL_StatusTypeDef status = I2C_For_GDKG_ReadRegisters(GDKG_I2C_REG_VELOCITY_MPS, buffer, 4);
    if (status != HAL_OK) {
        return status;
    }
    
    // 解析速度数据
    if (velocity_mps != NULL) {
        uint16_t mps_raw = (buffer[0] << 8) | buffer[1];
        *velocity_mps = mps_raw / 100.0f; // 转换为浮点数
    }
    
    if (velocity_kmph != NULL) {
        uint16_t kmph_raw = (buffer[2] << 8) | buffer[3];
        *velocity_kmph = kmph_raw / 100.0f; // 转换为浮点数
    }
    
    return HAL_OK;
}

/**
 * @brief 读取原始速度数据
 * @param buffer 缓冲区
 * @param size 缓冲区大小
 * @retval HAL_OK 成功
 */
HAL_StatusTypeDef I2C_For_GDKG_ReadVelocityRaw(uint8_t* buffer, uint8_t size)
{
    if (size < 4) {
        return HAL_ERROR;
    }
    
    return I2C_For_GDKG_ReadRegisters(GDKG_I2C_REG_VELOCITY_MPS, buffer, 4);
}

/**
 * @brief 读取脉冲计数
 * @param pulse_count 脉冲计数输出指针
 * @retval HAL_OK 成功
 */
HAL_StatusTypeDef I2C_For_GDKG_ReadPulseCount(uint32_t* pulse_count)
{
    uint8_t buffer[4];
    
    HAL_StatusTypeDef status = I2C_For_GDKG_ReadRegisters(GDKG_I2C_REG_PULSE_COUNT, buffer, 4);
    if (status != HAL_OK) {
        return status;
    }
    
    if (pulse_count != NULL) {
        *pulse_count = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
    }
    
    return status;
}

/**
 * @brief 读取状态寄存器
 * @param status 状态输出指针
 * @retval HAL_OK 成功
 */
HAL_StatusTypeDef I2C_For_GDKG_ReadStatus(uint8_t* status)
{
    return I2C_For_GDKG_ReadRegister(GDKG_I2C_REG_STATUS, status);
}

/**
 * @brief 读取设备ID
 * @param device_id 设备ID输出指针
 * @retval HAL_OK 成功
 */
HAL_StatusTypeDef I2C_For_GDKG_ReadDeviceID(uint8_t* device_id)
{
    return I2C_For_GDKG_ReadRegister(GDKG_I2C_REG_DEVICE_ID, device_id);
}

/**
 * @brief 重置脉冲计数
 * @retval HAL_OK 成功
 */
HAL_StatusTypeDef I2C_For_GDKG_ResetPulseCount(void)
{
    return I2C_For_GDKG_WriteRegister(GDKG_I2C_REG_CTRL, GDKG_CTRL_RESET_PULSE);
}

/**
 * @brief 开始测量
 * @retval HAL_OK 成功
 */
HAL_StatusTypeDef I2C_For_GDKG_StartMeasurement(void)
{
    return I2C_For_GDKG_WriteRegister(GDKG_I2C_REG_CTRL, GDKG_CTRL_START_MEASURE);
}

/**
 * @brief 停止测量
 * @retval HAL_OK 成功
 */
HAL_StatusTypeDef I2C_For_GDKG_StopMeasurement(void)
{
    return I2C_For_GDKG_WriteRegister(GDKG_I2C_REG_CTRL, GDKG_CTRL_STOP_MEASURE);
}

/**
 * @brief 获取错误计数
 * @retval 错误计数
 */
uint32_t I2C_For_GDKG_GetErrorCount(void)
{
    return s_error_count;
}

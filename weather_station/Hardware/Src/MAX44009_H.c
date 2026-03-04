#include <math.h>
#include "main.h"
#include "MAX44009_H.h"

typedef struct {
    I2C_HandleTypeDef handle;
    bool initialized;
} max44009_bus_t;

static max44009_bus_t s_bus = {0};

static void MAX44009_I2C_Init(void)
{
    if (s_bus.initialized) return;

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_I2C_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_I2C;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    __HAL_RCC_I2C_FORCE_RESET();
    __HAL_RCC_I2C_RELEASE_RESET();

    s_bus.handle.Instance             = I2C;
    s_bus.handle.Init.ClockSpeed      = 30000;
    s_bus.handle.Init.DutyCycle       = I2C_DUTYCYCLE_16_9;
    s_bus.handle.Init.OwnAddress1     = MAX44009_ADDRESS;
    s_bus.handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    s_bus.handle.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&s_bus.handle) != HAL_OK) {
        while (1);
    }

    s_bus.initialized = true;
}

static void max44009_write_u8(uint8_t devaddr, uint8_t reg, uint8_t data)
{
    uint8_t buf = data;
    if (HAL_I2C_Mem_Write(&s_bus.handle, devaddr, reg, I2C_MEMADD_SIZE_8BIT, &buf, 1, 1000) != HAL_OK) {
        while (1);
    }
    while (HAL_I2C_GetState(&s_bus.handle) != HAL_I2C_STATE_READY);
}

static void max44009_read_u8(uint8_t devaddr, uint8_t reg, uint8_t* data)
{
    if (HAL_I2C_Mem_Read(&s_bus.handle, devaddr, reg, I2C_MEMADD_SIZE_8BIT, data, 1, 1000) != HAL_OK) {
        while (1);
    }
    while (HAL_I2C_GetState(&s_bus.handle) != HAL_I2C_STATE_READY);
}

void MAX44009_Init(void)
{
    MAX44009_I2C_Init();
    HAL_Delay(20);

    max44009_write_u8(MAX44009_ADDRESS, MAX44009_REG_CFG, 0x03);
    max44009_write_u8(MAX44009_ADDRESS, MAX44009_REG_INTE, MAX44009_INT_DISABLE);
    max44009_write_u8(MAX44009_ADDRESS, MAX44009_REG_THT, MAX44009_IT_800ms);
    max44009_write_u8(MAX44009_ADDRESS, MAX44009_REG_THU, 0xFF);
    max44009_write_u8(MAX44009_ADDRESS, MAX44009_REG_THL, 0x00);
    max44009_write_u8(MAX44009_ADDRESS, MAX44009_REG_CFG, (uint8_t)(MAX44009_CFG_CONT | MAX44009_CFG_MANUAL | MAX44009_IT_6d25ms));
}

void MAX44009GetData(float* lux_value)
{
    MAX44009_I2C_Init();
    uint8_t high = 0, low = 0;
    max44009_read_u8(MAX44009_ADDRESS, MAX44009_REG_LUXH, &high);
    max44009_read_u8(MAX44009_ADDRESS, MAX44009_REG_LUXL, &low);

    uint8_t exponent = (high & 0xF0) >> 4;
    uint8_t mantissa = (uint8_t)((high & 0x0F) << 4) | (low & 0x0F);

    float result = ((float)(1U << exponent) * (float)mantissa) * 0.045f;
    *lux_value = result;
}

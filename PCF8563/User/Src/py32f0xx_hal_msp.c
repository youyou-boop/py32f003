/**
  ******************************************************************************
  * @file    py32f0xx_hal_msp.c
  * @author  MCU SYSTEM Team
  * @Version V1.0.0
  * @Date    2020-10-19
  * @brief   This file provides code for the MSP Initialization
  *          and de-Initialization codes.
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "py32f0xx_hal_i2c.h"
#include "py32f0xx_hal_gpio_ex.h"


/**
  * Initializes the Global MSP.
  */

void HAL_MspInit(void)
{
    /* USER CODE BEGIN MspInit 0 */
    /* USER CODE END MspInit 0 */
}

void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_I2C_CLK_ENABLE();
        GPIO_InitTypeDef gi = {0};
        gi.Pin = GPIO_PIN_6 | GPIO_PIN_7;
        gi.Mode = GPIO_MODE_AF_OD;
        gi.Pull = GPIO_PULLUP;
        gi.Speed = GPIO_SPEED_FREQ_HIGH;
        gi.Alternate = GPIO_AF6_I2C;
        HAL_GPIO_Init(GPIOB, &gi);
    }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        __HAL_RCC_I2C_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6 | GPIO_PIN_7);
    }
}


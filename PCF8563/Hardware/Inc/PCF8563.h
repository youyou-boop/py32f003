#ifndef __PCF8563_H
#define __PCF8563_H

#include "main.h"
#include "i2c.h"

/* 7bit I2C 地址（不含 R/W 位） */
#define PCF8563_I2C_ADDR         0x51U

/* 关键寄存器地址 */
#define PCF8563_REG_CTRL1        0x00U   /* Control/Status1：STOP 位在此寄存器，0=运行，1=停止 */
#define PCF8563_REG_CTRL2        0x01U   /* Control/Status2：中断/标志控制 */
#define PCF8563_REG_SECONDS      0x02U   /* 秒寄存器，bit7 为 VL 低电压指示 */
#define PCF8563_REG_MINUTES      0x03U
#define PCF8563_REG_HOURS        0x04U
#define PCF8563_REG_DAYS         0x05U
#define PCF8563_REG_WEEKDAYS     0x06U
#define PCF8563_REG_MONTHS       0x07U   /* bit7 为世纪位 C */
#define PCF8563_REG_YEARS        0x08U
#define PCF8563_REG_CLKOUT       0x0DU   /* CLKOUT 控制寄存器 */

/* 闹钟寄存器与位定义 */
#define PCF8563_REG_MIN_ALARM    0x09U   /* bit7 = AE (1=不比较该域) */
#define PCF8563_REG_HOUR_ALARM   0x0AU   /* bit7 = AE */
#define PCF8563_REG_DAY_ALARM    0x0BU   /* bit7 = AE */
#define PCF8563_REG_WDAY_ALARM   0x0CU   /* bit7 = AE */
#define PCF8563_ALARM_AE         0x80U

/* Control/Status2 位（基于 PCF8563 典型定义） */
#define PCF8563_CTRL2_TI_TP      (1U<<7) /* 定时器输出脉冲/电平选择 */
#define PCF8563_CTRL2_AF         (1U<<6) /* 闹钟标志 */
#define PCF8563_CTRL2_TF         (1U<<5) /* 定时器标志 */
#define PCF8563_CTRL2_AIE        (1U<<4) /* 闹钟中断使能 */
#define PCF8563_CTRL2_TIE        (1U<<3) /* 定时器中断使能 */

/* INT 引脚定义（默认 PB5） */
#define PCF8563_INT_GPIO_PORT    GPIOB
#define PCF8563_INT_PIN          GPIO_PIN_5
#define PCF8563_INT_IRQn         EXTI4_15_IRQn

/* API */
void PCF8563_Init(void);
HAL_StatusTypeDef PCF8563_Write(uint8_t reg, const uint8_t *data, uint16_t len, uint32_t timeout);
HAL_StatusTypeDef PCF8563_Read(uint8_t reg, uint8_t *data, uint16_t len, uint32_t timeout);

/* 时间结构体（年份为 0~99，世纪位不做扩展；VL=1 表示曾掉电，时间可能无效） */
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t weekday;   /* 0~6 对应周日~周六，依据你的系统约定 */
    uint8_t month;     /* 1~12 */
    uint8_t year;      /* 0~99 */
    uint8_t vl_flag;   /* 秒寄存器 bit7：1=低电压/时间无效 */
} PCF8563_Time;

/* 设置/读取时间（内部自动进行 BCD 编解码，写入时清除 VL 位与世纪位） */
HAL_StatusTypeDef PCF8563_SetTime(const PCF8563_Time *t);
HAL_StatusTypeDef PCF8563_GetTime(PCF8563_Time *t);

/* 闹钟相关接口 */
HAL_StatusTypeDef PCF8563_SetDailyAlarm(uint8_t hour, uint8_t minute);
HAL_StatusTypeDef PCF8563_EnableAlarmInterrupt(uint8_t enable);
HAL_StatusTypeDef PCF8563_ClearAlarmFlag(void);
HAL_StatusTypeDef PCF8563_GetAlarmFlag(uint8_t *af_set);

/* INT 相关：由驱动内部配置 PB5 外部中断，并提供统一的中断处理入口 */
void PCF8563_INT_Init(void);
void PCF8563_AlarmIRQHandler(void);
HAL_StatusTypeDef PCF8563_ReadRaw7(uint8_t *buf);

#endif 


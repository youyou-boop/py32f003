#ifndef __MAX44009_H
#define __MAX44009_H

#include <stdint.h>
#include <stdbool.h>

/* 设备地址选择 */
#define MAX44009_ADDR_G ((uint8_t)0x4A) /* A0 接地 */
#define MAX44009_ADDR_V ((uint8_t)0x4B) /* A0 接 VCC */
#define MAX44009_ADDRESS (MAX44009_ADDR_G << 1)

/* 寄存器 */
#define MAX44009_REG_INTS ((uint8_t)0x00)
#define MAX44009_REG_INTE ((uint8_t)0x01)
#define MAX44009_REG_CFG  ((uint8_t)0x02)
#define MAX44009_REG_LUXH ((uint8_t)0x03)
#define MAX44009_REG_LUXL ((uint8_t)0x04)
#define MAX44009_REG_THU  ((uint8_t)0x05)
#define MAX44009_REG_THL  ((uint8_t)0x06)
#define MAX44009_REG_THT  ((uint8_t)0x07)

/* 配置位 */
#define MAX44009_INT_ENABLE   ((uint8_t)0x01)
#define MAX44009_INT_DISABLE  ((uint8_t)0x00)
#define MAX44009_CFG_CONT     ((uint8_t)0x80)
#define MAX44009_CFG_MANUAL   ((uint8_t)0x40)
#define MAX44009_CDR_NORM     ((uint8_t)0x00)
#define MAX44009_CDR_DIV8     ((uint8_t)0x08)

/* 积分时间 */
#define MAX44009_IT_800ms     ((uint8_t)0x00)
#define MAX44009_IT_400ms     ((uint8_t)0x01)
#define MAX44009_IT_200ms     ((uint8_t)0x02)
#define MAX44009_IT_100ms     ((uint8_t)0x03)
#define MAX44009_IT_50ms      ((uint8_t)0x04)
#define MAX44009_IT_25ms      ((uint8_t)0x05)
#define MAX44009_IT_12d5ms    ((uint8_t)0x06)
#define MAX44009_IT_6d25ms    ((uint8_t)0x07)

/* 驱动接口 */
void MAX44009_Init(void);
void MAX44009GetData(float* lux_value);

#endif

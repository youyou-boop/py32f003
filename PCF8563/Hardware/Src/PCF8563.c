#include "PCF8563.h"

/* --- BCD 编解码工具 --- */
static uint8_t bcd2bin(uint8_t bcd){
    return (uint8_t)(((bcd >> 4) & 0x0F) * 10U + (bcd & 0x0F));
}

static uint8_t bin2bcd(uint8_t bin){
    return (uint8_t)(((bin / 10U) << 4) | (bin % 10U));
}

/* 配置 INT: PB5 下降沿中断（INT 为低有效开漏） */
void PCF8563_INT_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef gi = {0};
    gi.Pin  = PCF8563_INT_PIN;
    gi.Mode = GPIO_MODE_IT_FALLING;
    gi.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(PCF8563_INT_GPIO_PORT, &gi);
    HAL_NVIC_SetPriority(PCF8563_INT_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(PCF8563_INT_IRQn);
}

HAL_StatusTypeDef PCF8563_Write(uint8_t reg, const uint8_t *data, uint16_t len, uint32_t timeout){
    return I2C_WriteReg(PCF8563_I2C_ADDR, reg, data, len, timeout);
}

HAL_StatusTypeDef PCF8563_Read(uint8_t reg, uint8_t *data, uint16_t len, uint32_t timeout){

    return I2C_ReadReg(PCF8563_I2C_ADDR, reg, data, len, timeout);
}

/* 初始化 PCF8563：
 * 1) 清除 STOP 位，启动 32.768kHz 振荡器；
 * 2) 关闭/清除中断与标志；
 * 3) 关闭 CLKOUT（如需输出频率可自行配置）。
 * 使用方法：先调用 I2C_Init()，再调用本函数。
 */
void PCF8563_Init(void){
    uint8_t tmp;
    if (PCF8563_Read(PCF8563_REG_SECONDS, &tmp, 1U, 100U) != HAL_OK){
        return;
    }
    uint8_t ctrl1 = 0x00U;
    if (PCF8563_Write(PCF8563_REG_CTRL1, &ctrl1, 1U, 100U) != HAL_OK){
        return;
    }

    /* 初始化闹钟寄存器为全部忽略（AE=1），避免上电时的意外匹配 */
    uint8_t alm_ae[4] = {0x80U, 0x80U, 0x80U, 0x80U};
   (void)PCF8563_Write(PCF8563_REG_MIN_ALARM, alm_ae, sizeof(alm_ae), 100U);

    /* 清标志并暂不使能中断：CTRL2=0x00（清 AF/TF，AIE/TIE 关闭） */
    uint8_t ctrl2 = 0x00U;
    (void)PCF8563_Write(PCF8563_REG_CTRL2, &ctrl2, 1U, 100U);

    /* CLKOUT 控制：0x00 -> 关闭时钟输出（CO 引脚），如需 1Hz/32Hz/1kHz/32kHz 可改写 */
    uint8_t clkout = 0x00U;
    (void)PCF8563_Write(PCF8563_REG_CLKOUT, &clkout, 1U, 100U);
}

/* 读取时间（7 个连续寄存器，从秒开始），自动去除 VL/C 等标志位并做 BCD 转换 */
HAL_StatusTypeDef PCF8563_GetTime(PCF8563_Time *t){
    if (t == NULL) return HAL_ERROR;
    uint8_t buf[7] = {0};
    HAL_StatusTypeDef ret = PCF8563_Read(PCF8563_REG_SECONDS, buf, sizeof(buf), 100U);
    if (ret != HAL_OK) return ret;

    /* bit7: VL 标志（秒寄存器） */
    t->vl_flag = (buf[0] & 0x80U) ? 1U : 0U;
    t->seconds = bcd2bin(buf[0] & 0x7FU);
    t->minutes = bcd2bin(buf[1] & 0x7FU);
    t->hours   = bcd2bin(buf[2] & 0x3FU);
    t->day     = bcd2bin(buf[3] & 0x3FU);
    t->weekday = bcd2bin(buf[4] & 0x07U);
    t->month   = bcd2bin(buf[5] & 0x1FU);  /* 忽略世纪位 */
    t->year    = bcd2bin(buf[6]);
    return HAL_OK;
}

/* 设置时间（7 个连续寄存器，从秒开始），写入时清 VL/C 位 */
HAL_StatusTypeDef PCF8563_SetTime(const PCF8563_Time *t){
    if (t == NULL) return HAL_ERROR;

    /* 基本范围检查（越界时返回错误） */
    if (t->seconds > 59U || t->minutes > 59U || t->hours > 23U ||
        t->day == 0U || t->day > 31U || t->weekday > 6U ||
        t->month == 0U || t->month > 12U){
        return HAL_ERROR;
    }

    uint8_t buf[7];
    buf[0] = bin2bcd(t->seconds) & 0x7FU;           /* 清 VL 位 */
    buf[1] = bin2bcd(t->minutes) & 0x7FU;
    buf[2] = bin2bcd(t->hours)   & 0x3FU;
    buf[3] = bin2bcd(t->day)     & 0x3FU;
    buf[4] = bin2bcd(t->weekday) & 0x07U;
    buf[5] = bin2bcd(t->month)   & 0x1FU;           /* 清世纪位 C */
    buf[6] = bin2bcd(t->year);

    return PCF8563_Write(PCF8563_REG_SECONDS, buf, sizeof(buf), 100U);
}

/* 仅按“每天某时：某分”产生闹钟：秒级不可比，日与星期忽略（AE=1） */
HAL_StatusTypeDef PCF8563_SetDailyAlarm(uint8_t hour, uint8_t minute){
    if (hour > 23U || minute > 59U) return HAL_ERROR;
    uint8_t buf[4];
    buf[0] = bin2bcd(minute) & 0x7FU;        /* 分钟：bit7=0 比较有效 */
    buf[1] = bin2bcd(hour)   & 0x3FU;        /* 小时：bit7=0 比较有效 */
    buf[2] = 0x80U;                           /* 日：AE=1 不比较 */
    buf[3] = 0x80U;                           /* 星期：AE=1 不比较 */
    return PCF8563_Write(PCF8563_REG_MIN_ALARM, buf, sizeof(buf), 100U);
}

/* 使能/关闭闹钟中断输出（INT 引脚 PB5 ），并不清标志位 */
HAL_StatusTypeDef PCF8563_EnableAlarmInterrupt(uint8_t enable){
    uint8_t v;
    if (PCF8563_Read(PCF8563_REG_CTRL2, &v, 1U, 100U) != HAL_OK) return HAL_ERROR;
    if (enable) v |= PCF8563_CTRL2_AIE; else v &= (uint8_t)~PCF8563_CTRL2_AIE;
    return PCF8563_Write(PCF8563_REG_CTRL2, &v, 1U, 100U);
}

/* 读取并清除 AF 标志（写 0 清除） */
HAL_StatusTypeDef PCF8563_GetAlarmFlag(uint8_t *af_set){
    if (af_set == NULL) return HAL_ERROR;
    uint8_t v;
    if (PCF8563_Read(PCF8563_REG_CTRL2, &v, 1U, 100U) != HAL_OK) return HAL_ERROR;
    *af_set = (uint8_t)((v & PCF8563_CTRL2_AF) ? 1U : 0U);
    return HAL_OK;
}

HAL_StatusTypeDef PCF8563_ClearAlarmFlag(void){
    uint8_t v;
    if (PCF8563_Read(PCF8563_REG_CTRL2, &v, 1U, 100U) != HAL_OK) return HAL_ERROR;
    v &= (uint8_t)~PCF8563_CTRL2_AF;
    return PCF8563_Write(PCF8563_REG_CTRL2, &v, 1U, 100U);
}


/* 统一的闹钟中断处理：清 AF 并在 08:00/20:00 间切换下一次闹钟 */
void PCF8563_AlarmIRQHandler(void)
{
    static uint8_t toggle = 0;
    (void)PCF8563_ClearAlarmFlag();
    if (toggle == 0){
        (void)PCF8563_SetDailyAlarm(20, 0);
        toggle = 1;
    }else{
        (void)PCF8563_SetDailyAlarm(8, 0);
        toggle = 0;
    }
}

HAL_StatusTypeDef PCF8563_ReadRaw7(uint8_t *buf){
    if (buf == NULL) return HAL_ERROR;
    return PCF8563_Read(PCF8563_REG_SECONDS, buf, 7U, 100U);
}

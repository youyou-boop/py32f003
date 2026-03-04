#ifndef __AS5600_H__
#define __AS5600_H__

/**** 头文件 ****/
#include "main.h"

/**** 宏定义 ****/
#define IIC_SCL				GPIO_PIN_1
#define IIC_SDA				GPIO_PIN_0

/* AS5600寄存器地址*/
#define AS5600_ID_ADDR           (0x36)     /* IIC 从机地址*/
/* Only Read Reg */
#define AS5600_RAWAngleADDR_H   (0x0c)     /* Only low 4 bit */
#define AS5600_RAWAngleADDR_L   (0x0d)     /* original angle val */
#define AS5600_AngleADDR_H      (0x0E)     /* Only low 4 bit */
#define AS5600_AngleADDR_L      (0x0F)     /* Calculated angle val */
#define AS5600_STATUS            (0x0b)     /* Magnet status Reg */
#define AS5600_AGC               (0x1a)     /* Magnet status Reg */
#define AS5600_MAGNTUDE_H       (0x1b)     /* Only low 4 bit */
#define AS5600_MAGNTUDE_L       (0x1c)     /* Config Reg */
/* Only Write Reg*/
#define AS5600_BURN              (0xff)     /* Write EEPROM Reg */
/* R/W/P */
#define AS5600_ZMCO              (0x00)     /* Zero Magnetic Center offset */
#define AS5600_ZPOS_H            (0x01)     /* Only low 4 bit */
#define AS5600_ZPOS_L            (0x02)     /* Zero Postion */
#define AS5600_MPOS_H            (0x03)     /* Only low 4 bit */
#define AS5600_MPOS_L            (0x04)     /* Max Postion */
#define AS5600_MANG_H            (0x05)     /* Only low 4 bit */
#define AS5600_MANG_L            (0x06)     /* Max Angle */
#define AS5600_CONF_H            (0x07)     /* Only low 5 bit */
#define AS5600_CONF_L            (0x08)     /* Config Reg */

typedef enum /* 磁铁的状态*/
{
    MD =1,     // 检测到有磁铁且强度刚好
    ML =2,     // too weak
    MH =3      // too strong
}MagnetStatus;

/* 风向 */
typedef enum {
    WIND_N = 0,
    WIND_NNE,
    WIND_NE,
    WIND_ENE,
    WIND_E,
    WIND_ESE,
    WIND_SE,
    WIND_SSE,
    WIND_S,
    WIND_SSW,
    WIND_SW,
    WIND_WSW,
    WIND_W,
    WIND_WNW,
    WIND_NW,
    WIND_NNW
} WindDirection;


/**** 函数 ****/
void AS5600_Init(void);
uint8_t   AS5600_CheckDevice(void);

uint16_t  AS5600_GetAngleData(void);
uint16_t  AS5600_GetRAWAngleData(void);
float AS5600_GetAngle(void);
WindDirection AS5600_GetWindDirection(void);
const char* AS5600_WindDirectionToStr(WindDirection dir);

uint8_t   AS5600_GetMagnetStatus(void);
MagnetStatus AS5600_CheckMagnet(void);
uint8_t   AS5600_GetBurnTime(void);

/* 读写AS5600寄存器*/
void AS5600_WriteReg(uint16_t addr, uint8_t dat);
uint8_t   AS5600_ReadReg(uint16_t addr);

#endif // __AS5600_H__

#ifndef MODBUS_REGS_H
#define MODBUS_REGS_H

/* 设备地址寄存器 */
#define REG_DEVICE_ADDRESS              0x0010
/* 波特率寄存器 */
#define REG_BAUDRATE                    0x0020

#if defined(AS5600)

/* 角度校准寄存器 */
#define REG_WIND_ANGLE_CALIB_K          0x0040
#define REG_WIND_ANGLE_CALIB_B          0x0041
/* 角度寄存器 */
#define REG_WIND_ANGLE_HIGH             0x0050
/* 风向寄存器 */
#define REG_WIND_ANGLE_LOW              0x0051

#elif defined(BME280)

/* 温、湿、压寄存器 */
#define REG_TEMP                        0x0060
#define REG_HUMIDITY                    0x0061
#define REG_PRESSURE                    0x0062

/* 温湿度、压力校准寄存器 */
#define REG_TEMP_CALIB_K                0x0042
#define REG_TEMP_CALIB_B                0x0043
#define REG_HUMIDITY_CALIB_K            0x0044
#define REG_HUMIDITY_CALIB_B            0x0045
#define REG_PRESSURE_CALIB_K            0x0046
#define REG_PRESSURE_CALIB_B            0x0047

#elif defined(MAX44009)

/* 光照寄存器 */
#define REG_LUX                         0x0070

/* 光照校准寄存器 */
#define REG_LUX_CALIB_K                 0x0048
#define REG_LUX_CALIB_B                 0x0049

#elif defined(CO2)

/* 二氧化碳寄存器 */
#define REG_CO2						    					0x0080

/* 二氧化碳校准寄存器 */
#define REG_CO2_CALIB_K									0x0050
#define REG_CO2_CALIB_B									0x0051

#elif defined(WIND)

/* 风速寄存器 */
#define REG_WIND_SPEED                  0x0090
/* 风速等级寄存器 */
#define REG_WIND_GRADE                  0x0091

/* 风速校准寄存器 */
#define REG_WIND_SPEED_CALIB_K          0x0052
#define REG_WIND_SPEED_CALIB_B          0x0053

#endif

/* 恢复默认值寄存器 */
#define REG_RESET_DEFAULT               0x00FF


#endif

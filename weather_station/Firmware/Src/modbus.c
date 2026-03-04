#include "modbus.h"
#include <math.h>
#if defined(AS5600)
#include "AS5600.h"
#elif defined(BME280)
#include "bme280.h"
#elif defined(MAX44009)
#include "MAX44009_H.h"
#elif defined(WIND)
#include "gdkg.h"
#endif
#include "modbus_regs.h"
#include "USART1_RS485.h"
#include "py32f0xx_hal_flash.h"

// 静态变量存储Modbus地址
static uint8_t s_addr;
//static uint8_t s_baud_hi;
//static uint8_t s_baud_lo;
static uint8_t s_baud_code_saved;   	// 波特率代码（1..5），保存在Flash，断电后生效
static uint8_t s_baud_code_active;   	// 当前生效的波特率代码（1..5）

#if defined(AS5600)
static int16_t s_calib_k;
static int16_t s_calib_b;
#elif defined(BME280)
static int16_t s_temp_k;
static int16_t s_temp_b;
static int16_t s_humi_k;
static int16_t s_humi_b;
static int16_t s_pres_k;
static int16_t s_pres_b;
#elif defined(MAX44009)
static int16_t s_lux_k;
static int16_t s_lux_b;
#elif defined(CO2)
static int16_t s_co2_k;
static int16_t s_co2_b;
#elif defined(WIND)
static int16_t s_wind_k;
static int16_t s_wind_b;
#endif

/************************ 通用寄存器 *******************************/

/**
 * @brief 从Flash加载Modbus地址
 * @details 从MODBUS_FLASH_ADDR读取Modbus地址，校验CRC16和地址范围，无效则使用默认宏MODBUS_DEFAULT_ADDR
 */
static void load_addr_from_flash(void) {
    const uint32_t magic = 0x4D425341u; 										// 'MBSA' 用于识别数据有效性
    const uint8_t *p = (const uint8_t *)MODBUS_FLASH_ADDR; 	// 直接从Flash地址映射读取
    uint32_t m = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); // 读取magic（小端）
    uint8_t addr = p[4];  																	// 存储的设备地址
    uint8_t baud_code = p[5];   														// 存储的波特率代码（1..5），旧设备可能为0xFF
    uint16_t crc_stored = (uint16_t)p[6] | ((uint16_t)p[7] << 8); // 存储的CRC16（低字节在前）
    uint8_t tmp[6]; 																				// 计算CRC所用的数据缓冲区：magic(4)+addr(1)+baud_code(1)
    tmp[0] = (uint8_t)(magic & 0xFF);
    tmp[1] = (uint8_t)((magic >> 8) & 0xFF);
    tmp[2] = (uint8_t)((magic >> 16) & 0xFF);
    tmp[3] = (uint8_t)((magic >> 24) & 0xFF);
    tmp[4] = addr;
    tmp[5] = baud_code;
    uint16_t crc_calc = modbusCRC16(tmp, 6); 								// 按同样的格式计算CRC16校验
    if (m == magic && crc_calc == crc_stored && addr >= 1 && addr <= 247) { // 校验通过且地址在1..247范围（Modbus有效地址）
        s_addr = addr;
    } else {
        s_addr = MODBUS_DEFAULT_ADDR; 											// 回退到默认地址
    }
    if (m == magic && crc_calc == crc_stored && baud_code >= 1 && baud_code <= 5) {
        s_baud_code_saved = baud_code;                			// 载入待生效代码
    } else {
        s_baud_code_saved = 2;                         			// 默认代码：2 -> 9600
    }
    s_baud_code_active = s_baud_code_saved;          				// 上电时当前生效代码 = 保存代码（断电后生效）
    
#if defined(AS5600)
    // 载入角度校准参数：紧随地址/波特率之后
    uint16_t k_stored = (uint16_t)p[8] | ((uint16_t)p[9] << 8);
    uint16_t b_stored = (uint16_t)p[10] | ((uint16_t)p[11] << 8);
    // 
    s_calib_k = (k_stored == 0xFFFFu) ? (int16_t)1000 : (int16_t)k_stored;
    s_calib_b = (b_stored == 0xFFFFu) ? (int16_t)0    : (int16_t)b_stored;
#elif defined(BME280)
    // 载入温湿压校准参数（Temp, Humi, Pres），偏移12字节开始（前8字节头+4字节AS5600位置复用）
    // 为了兼容，我们使用偏移8开始的位置存储Temp，偏移12开始存Humi，偏移16开始存Pres
    uint16_t tk = (uint16_t)p[8] | ((uint16_t)p[9] << 8);
    uint16_t tb = (uint16_t)p[10] | ((uint16_t)p[11] << 8);
    uint16_t hk = (uint16_t)p[12] | ((uint16_t)p[13] << 8);
    uint16_t hb = (uint16_t)p[14] | ((uint16_t)p[15] << 8);
    uint16_t pk = (uint16_t)p[16] | ((uint16_t)p[17] << 8);
    uint16_t pb = (uint16_t)p[18] | ((uint16_t)p[19] << 8);

    s_temp_k = (tk == 0xFFFFu) ? (int16_t)1000 : (int16_t)tk;
    s_temp_b = (tb == 0xFFFFu) ? (int16_t)0    : (int16_t)tb;
    s_humi_k = (hk == 0xFFFFu) ? (int16_t)1000 : (int16_t)hk;
    s_humi_b = (hb == 0xFFFFu) ? (int16_t)0    : (int16_t)hb;
    s_pres_k = (pk == 0xFFFFu) ? (int16_t)1000 : (int16_t)pk;
    s_pres_b = (pb == 0xFFFFu) ? (int16_t)0    : (int16_t)pb;
#elif defined(MAX44009)
    // 载入MAX44009光照校准参数，偏移8字节开始
    uint16_t lk = (uint16_t)p[8] | ((uint16_t)p[9] << 8);
    uint16_t lb = (uint16_t)p[10] | ((uint16_t)p[11] << 8);

    s_lux_k = (lk == 0xFFFFu) ? (int16_t)1000 : (int16_t)lk;
    s_lux_b = (lb == 0xFFFFu) ? (int16_t)0    : (int16_t)lb;
#elif defined(CO2)
    // 载入CO2校准参数，偏移8字节开始
    uint16_t ck = (uint16_t)p[8] | ((uint16_t)p[9] << 8);
    uint16_t cb = (uint16_t)p[10] | ((uint16_t)p[11] << 8);

    s_co2_k = (ck == 0xFFFFu) ? (int16_t)1000 : (int16_t)ck;
    s_co2_b = (cb == 0xFFFFu) ? (int16_t)0    : (int16_t)cb;
#elif defined(WIND)
    // 载入风速校准参数，偏移8字节开始
    uint16_t wk = (uint16_t)p[8] | ((uint16_t)p[9] << 8);
    uint16_t wb = (uint16_t)p[10] | ((uint16_t)p[11] << 8);

    s_wind_k = (wk == 0xFFFFu) ? (int16_t)1000 : (int16_t)wk;
    s_wind_b = (wb == 0xFFFFu) ? (int16_t)0    : (int16_t)wb;
#endif
}

/**
 * @brief 保存地址/波特率/校准参数到Flash
 * @details
 * - 写入页布局：magic(4)+addr(1)+baud_code(1)+crc(2)+Params(...)
 * - 参数根据宏定义不同而不同：
 *   AS5600: K(2)+B(2) (offset 8)
 *   BME280: TempK(2)+TempB(2)+HumiK(2)+HumiB(2)+PresK(2)+PresB(2) (offset 8)
 * @param addr 设备地址
 * @param baud_code 保存（待生效）波特率代码
 * @param reset_default 是否恢复默认值（true则写入FFFF）
 */
static void save_flash_params(uint8_t addr, uint8_t baud_code, uint8_t reset_default) {
    const uint32_t magic = 0x4D425341u; 										// 'MBSA'
    uint8_t header[8];
    header[0] = (uint8_t)(magic & 0xFF);
    header[1] = (uint8_t)((magic >> 8) & 0xFF);
    header[2] = (uint8_t)((magic >> 16) & 0xFF);
    header[3] = (uint8_t)((magic >> 24) & 0xFF);
    header[4] = addr;
    header[5] = baud_code;
    uint16_t crc = modbusCRC16(header, 6); 										// 对前6字节计算CRC16
    header[6] = (uint8_t)(crc & 0xFF);
    header[7] = (uint8_t)(crc >> 8);
    
    FLASH_EraseInitTypeDef EraseInit = {0};
    uint32_t PageError = 0;
    
    // 关中断保护Flash操作
    __disable_irq();
    
    HAL_FLASH_Unlock();
    // 清除潜在的错误标志
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_OPTVERR);
    
    EraseInit.TypeErase = FLASH_TYPEERASE_PAGEERASE;
    EraseInit.PageAddress = MODBUS_FLASH_ADDR;
    EraseInit.NbPages = 1;
    if (HAL_FLASH_Erase(&EraseInit, &PageError) == HAL_OK) {
        uint8_t pageBytes[128];
        for (int i = 0; i < 128; i++) pageBytes[i] = 0xFF;
        for (int i = 0; i < 8; i++) pageBytes[i] = header[i];
        
        // 填充参数数据
#if defined(AS5600)
        uint16_t k = reset_default ? 0xFFFFu : (uint16_t)s_calib_k;
        uint16_t b = reset_default ? 0xFFFFu : (uint16_t)s_calib_b;
        pageBytes[8]  = (uint8_t)(k & 0xFF);
        pageBytes[9]  = (uint8_t)(k >> 8);
        pageBytes[10] = (uint8_t)(b & 0xFF);
        pageBytes[11] = (uint8_t)(b >> 8);
#elif defined(BME280)
        uint16_t tk = reset_default ? 0xFFFFu : (uint16_t)s_temp_k;
        uint16_t tb = reset_default ? 0xFFFFu : (uint16_t)s_temp_b;
        uint16_t hk = reset_default ? 0xFFFFu : (uint16_t)s_humi_k;
        uint16_t hb = reset_default ? 0xFFFFu : (uint16_t)s_humi_b;
        uint16_t pk = reset_default ? 0xFFFFu : (uint16_t)s_pres_k;
        uint16_t pb = reset_default ? 0xFFFFu : (uint16_t)s_pres_b;
        
        // Temp
        pageBytes[8]  = (uint8_t)(tk & 0xFF);
        pageBytes[9]  = (uint8_t)(tk >> 8);
        pageBytes[10] = (uint8_t)(tb & 0xFF);
        pageBytes[11] = (uint8_t)(tb >> 8);
        // Humi
        pageBytes[12] = (uint8_t)(hk & 0xFF);
        pageBytes[13] = (uint8_t)(hk >> 8);
        pageBytes[14] = (uint8_t)(hb & 0xFF);
        pageBytes[15] = (uint8_t)(hb >> 8);
        // Pres
        pageBytes[16] = (uint8_t)(pk & 0xFF);
        pageBytes[17] = (uint8_t)(pk >> 8);
        pageBytes[18] = (uint8_t)(pb & 0xFF);
        pageBytes[19] = (uint8_t)(pb >> 8);
#elif defined(MAX44009)
        uint16_t lk = reset_default ? 0xFFFFu : (uint16_t)s_lux_k;
        uint16_t lb = reset_default ? 0xFFFFu : (uint16_t)s_lux_b;
        pageBytes[8]  = (uint8_t)(lk & 0xFF);
        pageBytes[9]  = (uint8_t)(lk >> 8);
        pageBytes[10] = (uint8_t)(lb & 0xFF);
        pageBytes[11] = (uint8_t)(lb >> 8);
#elif defined(CO2)
        uint16_t ck = reset_default ? 0xFFFFu : (uint16_t)s_co2_k;
        uint16_t cb = reset_default ? 0xFFFFu : (uint16_t)s_co2_b;
        pageBytes[8]  = (uint8_t)(ck & 0xFF);
        pageBytes[9]  = (uint8_t)(ck >> 8);
        pageBytes[10] = (uint8_t)(cb & 0xFF);
        pageBytes[11] = (uint8_t)(cb >> 8);
#elif defined(WIND)
        uint16_t wk = reset_default ? 0xFFFFu : (uint16_t)s_wind_k;
        uint16_t wb = reset_default ? 0xFFFFu : (uint16_t)s_wind_b;
        pageBytes[8]  = (uint8_t)(wk & 0xFF);
        pageBytes[9]  = (uint8_t)(wk >> 8);
        pageBytes[10] = (uint8_t)(wb & 0xFF);
        pageBytes[11] = (uint8_t)(wb >> 8);
#endif

        uint32_t pageWords[32];
        for (int i = 0; i < 32; i++) {
            pageWords[i] = (uint32_t)pageBytes[i * 4]
                         | ((uint32_t)pageBytes[i * 4 + 1] << 8)
                         | ((uint32_t)pageBytes[i * 4 + 2] << 16)
                         | ((uint32_t)pageBytes[i * 4 + 3] << 24);
        }
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_PAGE, MODBUS_FLASH_ADDR, pageWords);
    }
    HAL_FLASH_Lock();
    
    // 恢复中断
    __enable_irq();
}

// 映射代码到实际波特率：1->4800, 2->9600, 3->19200, 4->38400, 5->115200
static uint32_t baud_from_code(uint8_t code){
    switch(code){
        case 1: return 4800;
        case 2: return 9600;
        case 3: return 19200;
        case 4: return 38400;
        case 5: return 115200;
        default: return 9600;
    }
}

/**
 * @brief 获取保存（待生效）波特率
 * @return uint32_t 保存（待生效）波特率
 */
uint32_t modbus_get_saved_baudrate(void){
    return baud_from_code(s_baud_code_saved);  					// 返回保存（待生效）波特率
}

void modbus_init(void) {
    load_addr_from_flash();   					// 从Flash读取设备地址、波特率与校准参数
}

#if defined(AS5600)

/************************ AS5600 *******************************/
/**
 * @brief 读取AS5600角度并转换为0.1°单位（0..3600）
 * @details 使用原始角度值(0..4095)，整数计算：(raw*3600+2048)/4096，含线性校准
 *          校准公式：val = raw*k + b，其中：
 *          k以千分标度存储（1.000 -> 1000），b以0.1°单位存储
 */
static uint16_t as5600_angle_tenth_deg(void) {
    uint16_t raw = AS5600_GetRAWAngleData() & 0x0FFF;
    uint32_t v = (uint32_t)raw * 3600u + 2048u;              // 四舍五入
    uint16_t deg10 = (uint16_t)(v / 4096u);
    int32_t d = (int32_t)deg10;
    d = (d * (int32_t)s_calib_k) / 1000;
    d = d + (int32_t)s_calib_b;
    // 归一化到0..3599（无符号角度），保证负偏移正确回绕，例如-10° -> 350°
    if (d >= 3600) d = d % 3600;
    if (d < 0) {
        d = d % 3600;
        if (d < 0) d += 3600;
    }
    return (uint16_t)d;
}

/**
 * @brief 读取风向编码
 * @details 返回编码：0x00=N, 0x01=NNE, ... 0x0F=NNW
 */
static uint16_t as5600_wind_code(void) {
    // 使用校准后的角度（0.1°单位）映射风向，每个方向22.5° -> 225单位
    uint16_t deg10 = as5600_angle_tenth_deg();
    uint16_t idx = ((uint16_t)(deg10 + 112u) / 225u) & 0x0Fu;   // 四舍五入到最近的风向区间
    return idx;
}

/**
 * @brief 读取“字节型”只读寄存器（0x0050..0x0053）
 * @param addr 寄存器地址
 * @param out 返回的单字节值
 * @return 0成功，-1地址非法
 * @details 这四个寄存器每个返回1字节，与标准16位保持不同的打包规则：
 *          0x0050: 风向角度高字节（0.1°单位的高8位）
 *          0x0051: 风向角度低字节（0.1°单位的低8位）
 *          0x0052: 风向编码高字节（固定0x00）
 *          0x0053: 风向编码低字节（0x00..0x0F）
 */
//static int reg_read_byte(uint16_t addr, uint8_t *out){
//    if (addr == REG_WIND_ANGLE_HIGH){
//        uint16_t v = as5600_angle_tenth_deg();
//        *out = (uint8_t)(v >> 8);
//        return 0;
//    }
//    if (addr == REG_WIND_ANGLE_LOW){
//        uint16_t v = as5600_angle_tenth_deg();
//        *out = (uint8_t)(v & 0xFF);
//        return 0;
//    }
//    if (addr == REG_WIND_HIGH){
//        *out = 0x00;
//        return 0;
//    }
//    if (addr == REG_WIND_LOW){
//        uint16_t c = as5600_wind_code();
//        *out = (uint8_t)(c & 0xFF);
//        return 0;
//    }
//    return -1;
//}

#elif defined(BME280)

/************************ BME280 *******************************/

/**
 * 说明：将BME280测得的温度/湿度/气压转换为Modbus寄存器值并支持线性校准
 * 寄存器尺度：
 * - REG_TEMP: 温度，单位0.01°C，支持负数（有符号16位）
 * - REG_HUMIDITY: 湿度，单位0.1%RH（0..1000）
 * - REG_PRESSURE: 气压，单位0.1 hPa（约3000..11000）
 * 线性校准：val = raw*k/1000 + b
 * - k以千分标度存储（默认k=1000表示不缩放）
 * - b与寄存器尺度一致（默认b=0）
 */
static int16_t s_temp_k = 1000,  s_temp_b = 0;
static int16_t s_humi_k = 1000,  s_humi_b = 0;
static int16_t s_pres_k = 1000,  s_pres_b = 0;

/* 读取一次并转换为寄存器尺度 */
static void bme280_read_scaled(int16_t *t, uint16_t *h, uint16_t *p) {
    float pressure = 0.0f, temperature = 0.0f, humidity = 0.0f, asl = 0.0f;
    bme280GetData(&pressure, &temperature, &humidity, &asl);
    int32_t ti = (int32_t)lroundf(temperature * 100.0f);        /* 温度0.01°C */
    ti = (ti * (int32_t)s_temp_k) / 1000 + (int32_t)s_temp_b;
    if (ti > 32767) ti = 32767;
    if (ti < -32768) ti = -32768;
    *t = (int16_t)ti;                                           /* 以二补码编码返回 */
    int32_t hi = (int32_t)lroundf(humidity * 10.0f);            /* 湿度0.1%RH */
    hi = (hi * (int32_t)s_humi_k) / 1000 + (int32_t)s_humi_b;
    if (hi < 0) hi = 0;
    if (hi > 65535) hi = 65535;
    *h = (uint16_t)hi;
    int32_t pi = (int32_t)lroundf(pressure * 10.0f);            /* 气压0.1 hPa */
    pi = (pi * (int32_t)s_pres_k) / 1000 + (int32_t)s_pres_b;
    if (pi < 0) pi = 0;
    if (pi > 65535) pi = 65535;
    *p = (uint16_t)pi;
}

#elif defined(MAX44009)

/************************ MAX44009 *******************************/

/**
 * @brief MAX44009 光照传感器参数与读取函数
 * 
 * 寄存器定义：
 * - REG_LUX (0x0070): 光照值，单位 0.1 Lux
 * - REG_LUX_CALIB_K (0x0048): 校准系数 K (默认1000)
 * - REG_LUX_CALIB_B (0x0049): 校准偏移 B (默认0)
 * 
 * 校准公式：Result = (Raw * 10 * K / 1000) + B
 */

static int16_t s_lux_k = 1000;  // 默认比例系数 1.0
static int16_t s_lux_b = 0;     // 默认偏移量 0

/**
 * @brief 读取光照数据并应用校准
 * @param lux [out] 输出校准后的光照值 (uint16_t, 单位 0.1 Lux)
 */
static void max44009_read_scaled(uint16_t *lux) {
    float f_lux = 0.0f;
    
    // 从传感器获取浮点光照值
    MAX44009GetData(&f_lux);
    
    // 转换为 0.1 Lux 单位 (乘以10后四舍五入)
    int32_t val = (int32_t)(f_lux * 10.0f + 0.5f);
    
    // 应用线性校准: val = val * k / 1000 + b
    val = (val * (int32_t)s_lux_k) / 1000 + (int32_t)s_lux_b;
    
    // 限制范围在 uint16_t 内
    if (val < 0) val = 0;
    if (val > 65535) val = 65535;
    
    *lux = (uint16_t)val;
}

#elif defined(CO2)

/************************ CO2 *******************************/

static uint16_t s_co2_val = 0;

/**
 * @brief 设置CO2寄存器值
 * @param val CO2浓度值
 */
void modbus_set_co2(uint16_t val) {
    int32_t v = (int32_t)val;
    v = (v * (int32_t)s_co2_k) / 1000 + (int32_t)s_co2_b;
    if (v < 0) v = 0;
    if (v > 65535) v = 65535;
    s_co2_val = (uint16_t)v;
}

#elif defined(WIND)

/************************ WIND *******************************/

static int16_t s_wind_k = 1000;  // 默认比例系数 1.0
static int16_t s_wind_b = 0;     // 默认偏移量 0

/**
 * @brief 根据风速计算风速等级 (Modbus内部逻辑)
 * @param v 风速(m/s)
 * @return uint16_t 风速等级
 */
static uint16_t calc_wind_grade(float v) {
    if (v <= 0.2f) return 0;
    else if (v <= 1.5f) return 1;
    else if (v <= 3.3f) return 2;
    else if (v <= 5.4f) return 3;
    else if (v <= 7.9f) return 4;
    else if (v <= 10.7f) return 5;
    else if (v <= 13.8f) return 6;
    else if (v <= 17.1f) return 7;
    else if (v <= 20.7f) return 8;
    else if (v <= 24.4f) return 9;
    else if (v <= 28.4f) return 10;
    else if (v <= 32.6f) return 11;
    else return 12;
}

/**
 * @brief 获取风速值 (放大10倍)
 * @details 将浮点风速转换为 0.1m/s 单位的整数，并应用校准
 *          校准公式：Result = (Raw * 10 * K / 1000) + B
 * @return uint16_t 风速值 (例如 12.3m/s -> 123)
 */
static uint16_t get_wind_speed_scaled(void) {
    // 限制最大值防止溢出
    if (speed_data.velocity_mps > 6553.5f) return 65535;
    
    // 转换为 0.1m/s 单位
    int32_t val = (int32_t)(speed_data.velocity_mps * 10.0f);
    
    // 应用校准: val = val * k / 1000 + b
    val = (val * (int32_t)s_wind_k) / 1000 + (int32_t)s_wind_b;
    
    if (val < 0) val = 0;
    if (val > 65535) val = 65535;
    
    return (uint16_t)val;
}

/**
 * @brief 获取风速等级
 * @return uint16_t 风速等级 (0-12)
 */
static uint16_t get_wind_grade(void) {
    // 使用校准后的风速计算等级
    uint16_t scaled = get_wind_speed_scaled();
    float v = (float)scaled / 10.0f;
    return calc_wind_grade(v);
}

#endif

/************************ modbus *******************************/

/**
 * @brief 更新Modbus CRC16校验值
 * @param crc 当前CRC16值
 * @param b 新字节
 * @return uint16_t 更新后的CRC16值
 */
static uint16_t crc_update(uint16_t crc, uint8_t b) {
    crc ^= b;
    for (uint8_t i = 0; i < 8; i++) {
        if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
        else crc >>= 1;
    }
    return crc;
}

/**
 * @brief 计算Modbus CRC16校验值
 * @param data 数据指针
 * @param len 数据长度
 * @return uint16_t CRC16校验值
 */
uint16_t modbusCRC16(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) crc = crc_update(crc, data[i]);
    return crc;
}

/**
 * @brief 读取Modbus寄存器值
 * @param addr 寄存器地址
 * @return uint16_t 寄存器值
 */
static uint16_t reg_read(uint16_t addr) {
    if (addr == REG_DEVICE_ADDRESS) {
        return (uint16_t)s_addr;   								// 读取设备地址寄存器（0x0010）
    }
    if (addr == REG_BAUDRATE) {
        return (uint16_t)s_baud_code_active;      // 返回当前生效代码 1..5（未断电前保持旧值）
    }
#if defined(AS5600)
    if (addr == REG_WIND_ANGLE_CALIB_K) {
        return s_calib_k;
    }
    if (addr == REG_WIND_ANGLE_CALIB_B) {
        return s_calib_b;
    }
    if (addr == REG_WIND_ANGLE_HIGH) {
        uint16_t v = as5600_angle_tenth_deg();
        return v;
    }
    if (addr == REG_WIND_ANGLE_LOW) {
        uint16_t c = as5600_wind_code();
        return c;
    }
#elif defined(BME280)
    /* 读取校准参数寄存器（用于查看当前k/b） */
    if (addr == REG_TEMP_CALIB_K)       return (uint16_t)s_temp_k;
    if (addr == REG_TEMP_CALIB_B)       return (uint16_t)s_temp_b;
    if (addr == REG_HUMIDITY_CALIB_K)   return (uint16_t)s_humi_k;
    if (addr == REG_HUMIDITY_CALIB_B)   return (uint16_t)s_humi_b;
    if (addr == REG_PRESSURE_CALIB_K)   return (uint16_t)s_pres_k;
    if (addr == REG_PRESSURE_CALIB_B)   return (uint16_t)s_pres_b;
    /* 读取温湿压寄存器 */
    if (addr == REG_TEMP) {
        int16_t t; uint16_t h, p;
        bme280_read_scaled(&t, &h, &p);
        return (uint16_t)t;                 /* 温度以二补码编码在16位寄存器中 */
    }
    if (addr == REG_HUMIDITY) {
        int16_t t; uint16_t h, p;
        bme280_read_scaled(&t, &h, &p);
        return h;
    }
    if (addr == REG_PRESSURE) {
        int16_t t; uint16_t h, p;
        bme280_read_scaled(&t, &h, &p);
        return p;
    }
#elif defined(MAX44009)
    /* 读取校准参数 */
    if (addr == REG_LUX_CALIB_K) return (uint16_t)s_lux_k;
    if (addr == REG_LUX_CALIB_B) return (uint16_t)s_lux_b;
    
    /* 读取光照值 */
    if (addr == REG_LUX) {
        uint16_t lux;
        max44009_read_scaled(&lux);
        return lux;
    }
#elif defined(CO2)
    /* 读取二氧化碳 */
    if (addr == REG_CO2) {
        return s_co2_val;
    }
    if (addr == REG_CO2_CALIB_K) return (uint16_t)s_co2_k;
    if (addr == REG_CO2_CALIB_B) return (uint16_t)s_co2_b;
#elif defined(WIND)
    /* 读取风速与等级 */
    if (addr == REG_WIND_SPEED) return get_wind_speed_scaled();
    if (addr == REG_WIND_GRADE) return get_wind_grade();
    if (addr == REG_WIND_SPEED_CALIB_K) return (uint16_t)s_wind_k;
    if (addr == REG_WIND_SPEED_CALIB_B) return (uint16_t)s_wind_b;
#endif
    return 0;
}

/**
 * @brief 构建Modbus异常响应
 * @param addr Modbus地址
 * @param func Modbus功能码
 * @param ex 异常码
 * @param rsp 响应数据缓冲区
 * @param rsp_len 响应数据长度指针
 * @return int 0成功
 */
static int build_exception(uint8_t addr, uint8_t func, uint8_t ex, uint8_t *rsp, uint16_t *rsp_len) {
    rsp[0] = addr;
    rsp[1] = func | 0x80;
    rsp[2] = ex;
    uint16_t crc = modbusCRC16(rsp, 3);
    rsp[3] = (uint8_t)(crc & 0xFF);
    rsp[4] = (uint8_t)(crc >> 8);
    *rsp_len = 5;
    return 0;
}

/**
 * @brief 写入Modbus寄存器值
 * @param addr 寄存器地址
 * @param val 寄存器值
 * @return int 0成功，-1地址错误
 */
static int reg_write(uint16_t addr, uint16_t val) {
    if (addr == REG_DEVICE_ADDRESS) {
        s_addr = (uint8_t)(val & 0xFF);  		// 写设备地址寄存器（低8位生效），立即切换后续通信地址
        // 保存到Flash：地址/波特率/当前k/b原样写入（二补码）
        save_flash_params(s_addr, s_baud_code_saved, 0);
        return 0;
    }
    if (addr == REG_BAUDRATE) {
        uint8_t code = (uint8_t)(val & 0xFF);
        if (code >= 1 && code <= 5){
            s_baud_code_saved = code;       // 更新待生效代码，不改变当前生效代码
            save_flash_params(s_addr, s_baud_code_saved, 0);     // 写入Flash，断电后生效
            return 0;
        }else{
            return -1;                      // 不合法代码
        }
    }
    if (addr == REG_RESET_DEFAULT) {
        if (val == 0x0001) {
            // “波特率编码”和“校准参数(k/b)”写入Flash为默认值，但不修改当前运行中的k/b。
            s_baud_code_saved = 2;                     // 默认波特率编码：2 -> 9600（掉电后生效）
            s_addr = MODBUS_DEFAULT_ADDR;              // 地址立即恢复默认
            save_flash_params(MODBUS_DEFAULT_ADDR, s_baud_code_saved, 1); // Flash中标记为默认参数，重启时应用
            return 0;
        } else {
            return -1;                                  // 非0x01的写入视为非法
        }
    }
#if defined(AS5600)
    if (addr == REG_WIND_ANGLE_CALIB_K) {
        s_calib_k = (int16_t)val;
        // 保存地址/波特率以及新的k/b到Flash，掉电后生效
        save_flash_params(s_addr, s_baud_code_saved, 0);
        return 0;
    }
    if (addr == REG_WIND_ANGLE_CALIB_B) {
        s_calib_b = (int16_t)val;
        // 保存地址/波特率以及新的k/b到Flash，掉电后生效
        save_flash_params(s_addr, s_baud_code_saved, 0);
        return 0;
    }

#elif defined(BME280)
    /* 校准参数写入（更新RAM并保存到Flash） */
    if (addr == REG_TEMP_CALIB_K)       { s_temp_k = (int16_t)val; save_flash_params(s_addr, s_baud_code_saved, 0); return 0; }
    if (addr == REG_TEMP_CALIB_B)       { s_temp_b = (int16_t)val; save_flash_params(s_addr, s_baud_code_saved, 0); return 0; }
    if (addr == REG_HUMIDITY_CALIB_K)   { s_humi_k = (int16_t)val; save_flash_params(s_addr, s_baud_code_saved, 0); return 0; }
    if (addr == REG_HUMIDITY_CALIB_B)   { s_humi_b = (int16_t)val; save_flash_params(s_addr, s_baud_code_saved, 0); return 0; }
    if (addr == REG_PRESSURE_CALIB_K)   { s_pres_k = (int16_t)val; save_flash_params(s_addr, s_baud_code_saved, 0); return 0; }
    if (addr == REG_PRESSURE_CALIB_B)   { s_pres_b = (int16_t)val; save_flash_params(s_addr, s_baud_code_saved, 0); return 0; }
    if (addr == REG_TEMP)               { return -1; }
    if (addr == REG_HUMIDITY)           { return -1; }
    if (addr == REG_PRESSURE)           { return -1; }
#elif defined(MAX44009)
    if (addr == REG_LUX_CALIB_K) {
        s_lux_k = (int16_t)val;
        save_flash_params(s_addr, s_baud_code_saved, 0);
        return 0;
    }
    if (addr == REG_LUX_CALIB_B) {
        s_lux_b = (int16_t)val;
        save_flash_params(s_addr, s_baud_code_saved, 0);
        return 0;
    }
    if (addr == REG_LUX) { return -1; }
#elif defined(CO2)
    if (addr == REG_CO2_CALIB_K) {
        s_co2_k = (int16_t)val;
        save_flash_params(s_addr, s_baud_code_saved, 0);
        return 0;
    }
    if (addr == REG_CO2_CALIB_B) {
        s_co2_b = (int16_t)val;
        save_flash_params(s_addr, s_baud_code_saved, 0);
        return 0;
    }
#elif defined(WIND)
    if (addr == REG_WIND_SPEED_CALIB_K) {
        // 更新待生效变量并写入Flash
        s_wind_k = (int16_t)val;
        save_flash_params(s_addr, s_baud_code_saved, 0); 
        return 0;
    }
    if (addr == REG_WIND_SPEED_CALIB_B) {
        // 更新待生效变量并写入Flash
        s_wind_b = (int16_t)val;
        save_flash_params(s_addr, s_baud_code_saved, 0);
        return 0;
    }
#endif
    return -1;
}

/**
 * @brief 检查寄存器是否可写
 * @param a 寄存器地址
 * @return int 1可写，0不可写
 */
static int is_writeable_reg(uint16_t a){
    if (a == REG_DEVICE_ADDRESS || a == REG_BAUDRATE || a == REG_RESET_DEFAULT) return 1;
#if defined(AS5600)
    if (a == REG_WIND_ANGLE_CALIB_K || a == REG_WIND_ANGLE_CALIB_B) return 1;
#endif
#if defined(BME280)
    if (a == REG_TEMP_CALIB_K 
			|| a == REG_TEMP_CALIB_B 
			|| a == REG_HUMIDITY_CALIB_K 
			|| a == REG_HUMIDITY_CALIB_B 
			|| a == REG_PRESSURE_CALIB_K 
			|| a == REG_PRESSURE_CALIB_B) return 1;
#elif defined(MAX44009)
    if (a == REG_LUX_CALIB_K || a == REG_LUX_CALIB_B) return 1;
#elif defined(CO2)
    if (a == REG_CO2_CALIB_K || a == REG_CO2_CALIB_B) return 1;
#elif defined(WIND)
    if (a == REG_WIND_SPEED_CALIB_K || a == REG_WIND_SPEED_CALIB_B) return 1;
#endif
    return 0;
}
/**
 * @brief 处理Modbus请求，返回响应长度
 * @param req Modbus请求数据
 * @param req_len 请求数据长度
 * @param rsp 响应数据缓冲区
 * @param rsp_len 响应数据长度指针
 * @return int 0成功，-1请求长度不足，-2校验错误，1地址错误，其他异常错误码
 */
int modbus_process(const uint8_t *req, uint16_t req_len, uint8_t *rsp, uint16_t *rsp_len) {
    *rsp_len = 0;
    if (req_len < 4) return -1;
    uint8_t addr = req[0];    															// 请求从站地址
    uint8_t func = req[1];    															// 请求功能码
    uint16_t crc_rx = (uint16_t)req[req_len - 2] | ((uint16_t)req[req_len - 1] << 8);
    uint16_t crc_calc = modbusCRC16(req, (uint16_t)(req_len - 2));
    if (crc_rx != crc_calc) return -2;
    if (addr != s_addr) return 1;
    if (func == 0x03) {         														// 功能码0x03查询寄存器
        if (req_len != 8) return build_exception(addr, func, 0x03, rsp, rsp_len), 0;
        uint16_t start = ((uint16_t)req[2] << 8) | req[3];  // 起始寄存器地址
        uint16_t qty = ((uint16_t)req[4] << 8) | req[5];    // 寄存器数量
        if (qty == 0 || qty > 0x007D) return build_exception(addr, func, 0x03, rsp, rsp_len), 0;
        for (uint16_t i = 0; i < qty; i++) {                // 检查寄存器地址是否合法
            uint16_t a = (uint16_t)(start + i);
            if (!(a == REG_DEVICE_ADDRESS
               || a == REG_BAUDRATE
#if defined(AS5600)
               || a == REG_WIND_ANGLE_CALIB_K
               || a == REG_WIND_ANGLE_CALIB_B
               || a == REG_WIND_ANGLE_HIGH
               || a == REG_WIND_ANGLE_LOW
#elif defined(BME280)
               || a == REG_TEMP_CALIB_K
               || a == REG_TEMP_CALIB_B
               || a == REG_HUMIDITY_CALIB_K
               || a == REG_HUMIDITY_CALIB_B
               || a == REG_PRESSURE_CALIB_K
               || a == REG_PRESSURE_CALIB_B
               || a == REG_TEMP
               || a == REG_HUMIDITY
               || a == REG_PRESSURE
#elif defined(MAX44009)
               || a == REG_LUX
               || a == REG_LUX_CALIB_K
               || a == REG_LUX_CALIB_B
#elif defined(CO2)
               || a == REG_CO2
               || a == REG_CO2_CALIB_K
               || a == REG_CO2_CALIB_B
#elif defined(WIND)
               || a == REG_WIND_SPEED
               || a == REG_WIND_GRADE
               || a == REG_WIND_SPEED_CALIB_K
               || a == REG_WIND_SPEED_CALIB_B
#endif
               )) {
                return build_exception(addr, func, 0x02, rsp, rsp_len), 0;
            }
        }
        rsp[0] = addr;
        rsp[1] = func;
        // 默认路径：每寄存器2字节
        rsp[2] = (uint8_t)(qty * 2);
        for (uint16_t i = 0; i < qty; i++) {
            uint16_t val = reg_read((uint16_t)(start + i));
            rsp[3 + i * 2] = (uint8_t)(val >> 8);
            rsp[4 + i * 2] = (uint8_t)(val & 0xFF);
        }
        uint16_t p = (uint16_t)(3 + qty * 2);     					// 响应数据区长度（不含CRC）
        uint16_t crc = modbusCRC16(rsp, p);       					// 计算响应CRC
        rsp[p] = (uint8_t)(crc & 0xFF);           					// CRC低字节
        rsp[p + 1] = (uint8_t)(crc >> 8);										// CRC高字节
        *rsp_len = (uint16_t)(p + 2);             					// 总响应长度=数据长度+CRC2字节
        return 0;
    } else if (func == 0x06) {         											// 功能码0x06写入单个寄存器
        if (req_len != 8) return build_exception(addr, func, 0x03, rsp, rsp_len), 0;
        uint16_t reg = ((uint16_t)req[2] << 8) | req[3];    // 目标寄存器地址
        uint16_t val = ((uint16_t)req[4] << 8) | req[5];    // 写入的寄存器值
        if (!is_writeable_reg(reg)) return build_exception(addr, func, 0x02, rsp, rsp_len), 0;
        if (reg_write(reg, val) != 0) return build_exception(addr, func, 0x02, rsp, rsp_len), 0;
        for (uint16_t i = 0; i < 6; i++) rsp[i] = req[i];
        uint16_t crc = modbusCRC16(rsp, 6);
        rsp[6] = (uint8_t)(crc & 0xFF);
        rsp[7] = (uint8_t)(crc >> 8);
        *rsp_len = 8;
        return 0;
    } else if (func == 0x10) {         				// 功能码0x10写入多个寄存器
        if (req_len < 9) return build_exception(addr, func, 0x03, rsp, rsp_len), 0;
        uint16_t start = ((uint16_t)req[2] << 8) | req[3];
        uint16_t qty = ((uint16_t)req[4] << 8) | req[5];
        uint8_t bc = req[6];
        if (qty == 0 || qty > 0x007B) return build_exception(addr, func, 0x03, rsp, rsp_len), 0;
        if (bc != qty * 2) return build_exception(addr, func, 0x03, rsp, rsp_len), 0;
        if (req_len != (uint16_t)(9 + bc)) return build_exception(addr, func, 0x03, rsp, rsp_len), 0;
        const uint8_t *pdata = &req[7];
        for (uint16_t i = 0; i < qty; i++) {
            uint16_t reg = (uint16_t)(start + i);
            if (!is_writeable_reg(reg)) return build_exception(addr, func, 0x02, rsp, rsp_len), 0;
        }
        for (uint16_t i = 0; i < qty; i++) {
            uint16_t val = ((uint16_t)pdata[i * 2] << 8) | pdata[i * 2 + 1];
            uint16_t reg = (uint16_t)(start + i);
            if (reg_write(reg, val) != 0) return build_exception(addr, func, 0x02, rsp, rsp_len), 0;
        }
        rsp[0] = addr;                          // 从站地址
        rsp[1] = func;                          // 功能码0x10
        rsp[2] = (uint8_t)(start >> 8);         // 起始地址高字节
        rsp[3] = (uint8_t)(start & 0xFF);       // 起始地址低字节
        rsp[4] = (uint8_t)(qty >> 8);           // 写入数量高字节
        rsp[5] = (uint8_t)(qty & 0xFF);         // 写入数量低字节
        uint16_t crc = modbusCRC16(rsp, 6);     // 计算响应CRC
        rsp[6] = (uint8_t)(crc & 0xFF);         // CRC低字节
        rsp[7] = (uint8_t)(crc >> 8);           // CRC高字节
        *rsp_len = 8;                           // 响应长度固定8字节
        return 0;
    } else {                           					// 返回错误处理
        return build_exception(addr, func, 0x01, rsp, rsp_len), 0;
    }
}

/**
 * @brief Modbus轮询处理：从串口接收一帧并应答（主操作函数）
 */
void modbus_poll(void) {
    uint8_t rx[128];
    uint16_t n = USART1_Receive(rx, sizeof(rx), 0);
    if (n > 0) {
        uint8_t tx[128];
        uint16_t tn = 0;
        int r = modbus_process(rx, n, tx, &tn);
        if (r == 0 && tn > 0) {
            USART1_Send_Byte(tx, tn);
        }
    }
}

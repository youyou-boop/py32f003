#ifndef MODBUS_H
#define MODBUS_H

#include <stdint.h>

#define MODBUS_DEFAULT_ADDR   0x01                  // 默认Modbus地址
#define MODBUS_FLASH_ADDR     0x08007F80            // PY32F003(32KB)最后一页地址(页大小128B)

void modbus_init(void); 
uint16_t modbusCRC16(const uint8_t *data, uint16_t len);    // 计算Modbus CRC16校验值
// 处理Modbus请求，返回响应长度
int modbus_process(const uint8_t *req, uint16_t req_len, uint8_t *rsp, uint16_t *rsp_len);

void modbus_poll(void);
uint32_t modbus_get_saved_baudrate(void);          // 读取Flash中保存的波特率并换算为实际数值

void modbus_set_co2(uint16_t val);                 // 设置CO2寄存器值

#endif

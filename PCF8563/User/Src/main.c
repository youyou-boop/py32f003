#include "main.h"
#include "RS485.h"
#include "i2c.h"
#include "PCF8563.h"
#include <stdio.h>

void Error_Handler(void){
	
    while (1){
			
    }
}

int main(void){
	
	HAL_Init();
  USART_Config(); 
  
  // 1. 暴力上电缓冲：给 PCF8563 2秒钟时间让电源稳定
  HAL_Delay(2000);
  
  // 2. 初始化 I2C
   I2C_Init();
   
   // 3. 初始化 PCF8563
   PCF8563_Init();

   // 4. 设置初始时间 (例如: 2023年10月27日 12:00:00 星期五)
   // 注意：PCF8563 的星期定义 0-6，具体对应关系取决于应用层约定，这里假设 5=Friday
   PCF8563_Time setTime = {
       .year = 23,
       .month = 10,
       .day = 27,
       .weekday = 5,
       .hours = 12,
       .minutes = 0,
       .seconds = 0,
       .vl_flag = 0 // 写入时此位无效，驱动会自动处理
   };
   
   if (PCF8563_SetTime(&setTime) != HAL_OK)
   {
       char err_msg[] = "Set Time Failed!\r\n";
       Modbus_Send_Byte((uint8_t*)err_msg, strlen(err_msg));
   }
   else
   {
       char ok_msg[] = "Set Time Success!\r\n";
       Modbus_Send_Byte((uint8_t*)ok_msg, strlen(ok_msg));
   }

   // 5. 循环读取时间并打印
    while (1)
    {
        PCF8563_Time now;
        HAL_StatusTypeDef ret = PCF8563_GetTime(&now);
        if (ret == HAL_OK)
        {
            char msg[64];
            // 格式化输出: 20YY-MM-DD HH:MM:SS Week:W
            snprintf(msg, sizeof(msg), "20%02d-%02d-%02d %02d:%02d:%02d Week:%d\r\n",
                     now.year, now.month, now.day,
                     now.hours, now.minutes, now.seconds,
                     now.weekday);
            Modbus_Send_Byte((uint8_t*)msg, strlen(msg));
        }
        else
        {
            char err_msg[64];
            snprintf(err_msg, sizeof(err_msg), "Read Time Failed! Error Code: %d\r\n", ret);
            Modbus_Send_Byte((uint8_t*)err_msg, strlen(err_msg));
            
            // 如果读取失败，尝试重新探测设备
            if (I2C_Probe(0x51, 10) == HAL_OK)
            {
                char msg[] = "Device 0x51 is still there.\r\n";
                Modbus_Send_Byte((uint8_t*)msg, strlen(msg));
            }
            else
            {
                char msg[] = "Device 0x51 is GONE!\r\n";
                Modbus_Send_Byte((uint8_t*)msg, strlen(msg));
            }
        }
        
        HAL_Delay(1000); // 1秒读一次
    }
}

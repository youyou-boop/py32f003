#include "main.h"
#include "USART1_RS485.h"
#include "modbus.h"
#include <stdio.h>

#if defined(AS5600)

#include "AS5600.h"

#elif defined(BME280)

#include "bme280.h"

#elif defined(MAX44009)

#include "MAX44009_H.h"

#elif defined(CO2)

#include "usart2_echo.h"

#elif defined(WIND)

#include "gdkg.h"
#include "i2c_for_gdkg.h"

#endif

void Error_Handler(void)
{
    while (1)
    {
    }
}

int main(void)
{
	HAL_Init();            //systick初始化
	
  modbus_init();    // 初始化Modbus地址与波特率：从Flash读取
  
#if defined(AS5600)
		AS5600_Init();
#elif defined(BME280)
		bme280Init();
#elif defined(MAX44009)
		MAX44009_Init();
#elif defined(CO2)	
	USART2_Echo_Init();

#elif defined(WIND)
	MX_GPIO_Init();
  MX_TIM3_Init();
	
	__enable_irq();// 启用全局中断
    
	HAL_TIM_Base_Start_IT(&htim3);// 启动定时器3
	
  if (HAL_TIM_Base_Start_IT(&htim3) != HAL_OK) {
      Error_Handler();}
	
#endif
	
	USART1_Init();
	
	while(1)
  {

#if defined(AS5600)
//		USART1_Send_Byte((uint8_t *)"usart1_RS485", 12);
//		
		// float angle = AS5600_GetAngle();			// 角度
		// int dir = AS5600_GetWindDirection();	// 风向
		
		// USART1_Send_String("angle=");
		// USART1_Send_Float2(angle);
		// USART1_Send_String("\r\n");
		
		// USART1_Send_String("dir=");
		// USART1_Send_String(AS5600_WindDirectionToStr(dir));
		// USART1_Send_String("\r\n");

#elif defined(BME280)

//			float pressure = 0.0f, temperature = 0.0f, humidity = 0.0f, asl = 0.0f;
//			bme280GetData(&pressure, &temperature, &humidity, &asl);
//			USART1_Send_String("temp=");
//			USART1_Send_Float2(temperature);
//			USART1_Send_String(" C, humi=");
//			USART1_Send_Float2(humidity);
//			USART1_Send_String(" %, pres=");
//			USART1_Send_Float2(pressure);
//			USART1_Send_String(" hPa\r\n");

#elif defined(MAX44009)

//			float lux = 0.0f;
//			MAX44009GetData(&lux);
//			USART1_Send_String("lux=");
//			USART1_Send_Float2(lux);
//			USART1_Send_String(" lx\r\n");

#elif defined(CO2)

	// USART1 -> USART2 & Modbus
		// static uint8_t us1_buf[128];
		// uint16_t us1_len = USART1_Receive(us1_buf, 128, 0);
		// if (us1_len > 0)
		// {
		// 	// 1. Forward to USART2
		// 	USART2_Send(us1_buf, us1_len);

		// 	// 2. Process Modbus
		// 	uint8_t tx[128];
		// 	uint16_t tn = 0;
		// 	int r = modbus_process(us1_buf, us1_len, tx, &tn);
		// 	if (r == 0 && tn > 0) {
		// 		USART1_Send_Byte(tx, tn);
		// 	}
		// }

		// // USART2 -> USART1
		// static uint8_t us2_buf[128];
		// uint16_t us2_len = USART2_ReceiveFrame(us2_buf, 128);
		// if (us2_len > 0)
		// {
		// 	USART1_Send_Byte(us2_buf, us2_len);
		// }
        
        if (USART2_Periodic_Send_Task())
        {
             // 提取 0A 29 (索引3和4)
             if (Response_Length >= 5) {
                 uint16_t co2_val = ((uint16_t)Response_Container[3] << 8) | Response_Container[4];
                 modbus_set_co2(co2_val);
             }
        }

#elif defined(WIND)
	// 不能注释
	GDKGGetData(NULL);		
	// 	USART1_Send_String("wind_speed=");
    // USART1_Send_Float2(speed_data.velocity_mps);
    // USART1_Send_String("\r\n");
	// 	USART1_Send_String("wind_grade=");
    // USART1_Send_Dec(speed_data.wind_grade);
    // USART1_Send_String("\r\n");

#endif
		
		modbus_poll();
    HAL_Delay(2); 	
	}
}

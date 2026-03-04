#include "main.h"
#include "RS485.h"

void Error_Handler(void){
	
    while (1){
			
    }
}

int main(void){
	
	HAL_Init();            //systick初始化
  USART_Config();        // 按加载的波特率初始化USART

	while(1){
		static uint32_t last = 0;
		static uint8_t which = 0;
		if (HAL_GetTick() - last >= 60000U){
			if (which == 0){
				uint8_t f1[] = {0xFF,0x01,0x00,0x07,0x00,0x01,0x09};
				Modbus_Send_Byte(f1, sizeof(f1));
				which = 1;
			}
			else{
				uint8_t f2[] = {0xFF,0x01,0x00,0x07,0x00,0x02,0x0A};
				Modbus_Send_Byte(f2, sizeof(f2));
				which = 0;
			}
		last = HAL_GetTick();
		}
	}
}

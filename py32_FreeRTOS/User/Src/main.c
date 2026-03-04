#include "main.h"
#include "usart2_echo.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

static TaskHandle_t StartTask_Handler = NULL;   // 启动任务句柄
static TaskHandle_t LedTask_Handler   = NULL;   // LED 任务句柄
static TaskHandle_t Usart1_Handler		= NULL;		// USART 任务句柄


// 由串口命令控制的LED状态（true=亮，false=灭）
static volatile bool g_led_on = false;

// LED 任务：根据 g_led_on 持续驱动 LED 引脚
static void led_task(void *pvParameters){
  (void)pvParameters;						// 避免警告
  while(1){
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, g_led_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(20));         // 周期性刷新，降低抖动与CPU占用
  }
}

//// USART 任务：回显接收字符
//static void usart_task(void *pvParameters){
//	(void)pvParameters;                       // 避免告警
////	const char u1[] = "USART2_RX";
//	//	USART2_Send((const uint8_t*)u1, (uint16_t)(sizeof(u1) - 1));	// 不能与回显同时使用
//	while(1){
//		USART2_Echo_Task();
//		vTaskDelay(pdMS_TO_TICKS(5));
//	}
//}

// 任务三：串口命令解析（on/off），控制 LED
// 串口帧以空闲中断(IDLE)划分；收到帧后解析字符串：
// - "on"  -> 点亮LED
// - "off" -> 关闭LED
// 可接受大小写，允许带有 \r \n
static void usart_cmd_task(void *pvParameters){
  (void)pvParameters;                       // 避免告警
  uint8_t  rxbuf[RxBuf_LENGHT2];
  char     cmd[RxBuf_LENGHT2];
//  const char ack_on[]  = "LED ON\r\n";
//  const char ack_off[] = "LED OFF\r\n";
  const char ack_err[] = "UNKNOWN CMD\r\n";

  while(1){
    // 等待一帧到达（由IDLE中断置位后返回长度>0）
    uint16_t n = USART2_ReceiveFrame(rxbuf, sizeof(rxbuf)-1);
    if (n > 0){
      // 组装为C字符串并去除结尾的\r\n
      if (n >= sizeof(cmd)) n = sizeof(cmd)-1;
      memcpy(cmd, rxbuf, n);
      cmd[n] = '\0';
      // 去掉尾部的 \r 和 \n
      while (n > 0 && (cmd[n-1] == '\r' || cmd[n-1] == '\n')){
        cmd[--n] = '\0';
      }
      // 转小写便于比较
      for (uint16_t i = 0; i < n; ++i){
        if (cmd[i] >= 'A' && cmd[i] <= 'Z') cmd[i] = (char)(cmd[i] - 'A' + 'a');
      }
      // 匹配命令
      if (strcmp(cmd, "on") == 0){
        g_led_on = false;
//        USART2_Send((const uint8_t*)ack_on, (uint16_t)(sizeof(ack_on)-1));
      } else if (strcmp(cmd, "off") == 0){
        g_led_on = true;
//        USART2_Send((const uint8_t*)ack_off, (uint16_t)(sizeof(ack_off)-1));
      } else {
        USART2_Send((const uint8_t*)ack_err, (uint16_t)(sizeof(ack_err)-1));
      }
    }
    // 适度让出CPU
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

// 启动任务：创建 LED 闪烁任务并删除自身
static void start_task(void *pvParameters){
  (void)pvParameters;						// 避免警告
  // 创建 LED 闪烁任务
  if (xTaskCreate((TaskFunction_t)led_task,
                  (const char*   )"led_task",
                  128,
                  NULL,
                  2,
                  (TaskHandle_t* )&LedTask_Handler) != pdPASS) {
    Error_Handler();
  }
	
  // 停止任务2，创建“任务三”串口命令任务
	if (xTaskCreate((TaskFunction_t)usart_cmd_task,
                (const char*   )"usart_cmd_task",
                192,
                NULL,
                1,
                (TaskHandle_t* )&Usart1_Handler) != pdPASS) {
    Error_Handler();
  }
							
  vTaskDelete(NULL); // 删除自身
}


void Error_Handler(void){
	
    while (1){
			
    }
}

void GPIO_LED_Config(void){
	
  __HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitTypeDef  GPIO_InitStruct;

	GPIO_InitStruct.Pin = GPIO_PIN_2;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
}

/**
 * @brief 内存分配失败钩子函数
 * 当FreeRTOS内存分配失败时，调用此函数。
 * 该函数会禁用中断并进入死循环，以防止系统继续运行。
 */
void vApplicationMallocFailedHook(void){
  taskDISABLE_INTERRUPTS();
  while(1){}
}

/**
 * @brief 堆栈溢出钩子函数
 * 当FreeRTOS任务堆栈溢出时，调用此函数。
 * 该函数会禁用中断并进入死循环，以防止系统继续运行。
 * @param xTask 溢出任务的句柄
 * @param pcTaskName 溢出任务的名称
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName){
  (void)xTask;
  (void)pcTaskName;
  taskDISABLE_INTERRUPTS();
  while(1){}
}

int main(void){
	
	HAL_Init();            //systick初始化
  GPIO_LED_Config();
  USART2_Echo_Init();        // 按加载的波特率初始化USART
	
	// Cortex-M0 无优先级分组配置，保持默认设置
    if (xTaskCreate((TaskFunction_t)start_task, 						// 任务函数指针
                              (const char*   )"start_task",			// 任务名称
                              128,									// 任务栈大小
                              NULL,									// 任务句柄
                              1, 										// 任务优先级
                              (TaskHandle_t*  )&StartTask_Handler) != pdPASS) {	// 任务句柄
      Error_Handler();
    }
    vTaskStartScheduler();											// 启动任务调度器

	while(1){
//		USART2_Echo_Task();
//		HAL_Delay(2);
    // 调度器启动后一般不会执行到这里
	}
}


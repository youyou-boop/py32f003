#include "main.h"
#include "usart2_echo.h"
#include "FreeRTOS.h"
#include "task.h"

static TaskHandle_t StartTask_Handler = NULL;   // 启动任务句柄
static TaskHandle_t LedTask_Handler   = NULL;   // LED 任务句柄
static TaskHandle_t Usart1_Handler		= NULL;		// USART 任务句柄

// LED 闪烁任务
static void led_task(void *pvParameters){
  (void)pvParameters;						// 避免警告
  while(1){
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_2);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

static void usart_task(void *pvParameters){
	(void)pvParameters;                       // 避免告警
	while(1){
		USART2_Echo_Task();                   
		vTaskDelay(pdMS_TO_TICKS(5));
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
	
	if (xTaskCreate((TaskFunction_t)usart_task,
                (const char*   )"usart_task",
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


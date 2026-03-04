#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* 基础配置 */
#define configUSE_PREEMPTION            1                // 使能抢占式调度
#define configUSE_IDLE_HOOK             0                // 使能空闲任务钩子
#define configUSE_TICK_HOOK             0                // 使能Tick任务钩子
#define configCPU_CLOCK_HZ              ( 8000000UL )  // 与当前SystemInit默认8MHz一致
#define configTICK_RATE_HZ              ( 1000 )       // 1ms 一个节拍
#define configMINIMAL_STACK_SIZE        ( 128 )        // 最小任务栈大小，根据任务需求调整
#define configTOTAL_HEAP_SIZE           ( 4096 )       // 堆大小，根据RAM调整
#define configMAX_TASK_NAME_LEN         ( 16 )         // 最大任务名长度
#define configUSE_TRACE_FACILITY        0                // 使能跟踪设施
#define configUSE_16_BIT_TICKS          0                // 使能16位节拍
#define configCHECK_FOR_STACK_OVERFLOW  2                // 检查栈溢出
#define configIDLE_SHOULD_YIELD         1                // 空闲任务是否应该让出CPU

/* 钩子函数（可选） */
#define configUSE_MALLOC_FAILED_HOOK    1                // 使能内存分配失败钩子
#define configUSE_APPLICATION_TASK_TAG  0                // 使能应用任务标签

/* 软件定时器（可选） */
#define configUSE_TIMERS                0                // 使能软件定时器
#define configTIMER_TASK_PRIORITY       2                // 定时器任务优先级
#define configTIMER_QUEUE_LENGTH        10               // 定时器队列长度
#define configTIMER_TASK_STACK_DEPTH    ( configMINIMAL_STACK_SIZE * 2 )    // 定时器任务栈深度

/* 互斥量（可选） */
#define configUSE_MUTEXES               0

/* 队列/信号量（可选） */
#define configUSE_COUNTING_SEMAPHORES   0
#define configUSE_ALTERNATIVE_API       0

/* 任务通知（可选） */
#define configUSE_TASK_NOTIFICATIONS    1

/* 低功耗模式（可选） */
#define configUSE_TICKLESS_IDLE         0

/* 任务优先级 */
#define configMAX_PRIORITIES            ( 5 )

/* 内存管理 */
#define configSUPPORT_DYNAMIC_ALLOCATION 1                // 使能动态内存分配
#define configSUPPORT_STATIC_ALLOCATION  0                // 使能静态内存分配

/* 断言 */
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/* 函数可用性：若使用到对应 API，需置 1 使能 */
#define INCLUDE_vTaskDelay              1
#define INCLUDE_vTaskDelete             1
#define INCLUDE_vTaskSuspend            0
#define INCLUDE_xTaskDelayUntil         0
#define INCLUDE_uxTaskPriorityGet       0
#define INCLUDE_vTaskPrioritySet        0
#define INCLUDE_xTaskGetSchedulerState  1

#endif /* FREERTOS_CONFIG_H */

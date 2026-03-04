#include <math.h>
#include "stdio.h"
#include "main.h"
#include "gdkg.h"
#include "i2c_for_gdkg.h"

static uint32_t s_current_pulse_count = 0;  // 当前秒的脉冲计数
//static uint32_t s_last_velocity_time = 0;   // 上次计算速度的时间

SpeedMeasurement_t speed_data = {0};
TIM_HandleTypeDef htim3;


// 系统时钟配置函数
void SystemClock_Config(void) {
    // 启用HSI
    RCC->CR |= RCC_CR_HSION;
    while((RCC->CR & RCC_CR_HSIRDY) == 0);
    
    // 配置AHB分频为1（不分频）
    RCC->CFGR &= ~(0xF << 4);  // 清除HPRE位
    
    // 配置APB1分频为1（不分频）
    RCC->CFGR &= ~(0x7 << 8);  // 清除PPRE位
    
    // 选择HSI作为系统时钟
    RCC->CFGR &= ~0x3;  // 清除SW位
    
    // 等待时钟切换完成
    while((RCC->CFGR & (0x3 << 2)) != 0);
    
    // 更新系统时钟变量
    SystemCoreClock = 8000000;
    }

// GPIO初始化
void MX_GPIO_Init(void) {
    GPIO_InitTypeDef gpio_init = {0};
    
    // 使能GPIOA时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // 配置PA12为外部中断输入
    gpio_init.Pin = PHOTO_SWITCH_PIN;
    gpio_init.Mode = GPIO_MODE_IT_FALLING;  // 使用GPIO_MODE_IT_FALLING
    gpio_init.Pull = GPIO_PULLDOWN;        // 下拉电阻
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PHOTO_SWITCH_PORT, &gpio_init);
    
    // 配置外部中断
    HAL_NVIC_SetPriority(PHOTO_SWITCH_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(PHOTO_SWITCH_IRQn);
}

// 定时器3初始化 - 1秒中断
void MX_TIM3_Init(void) {
    // 使能TIM3时钟
    __HAL_RCC_TIM3_CLK_ENABLE();
    
    // 配置TIM3
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 7999;  // 8MHz/(7999+1) = 1kHz
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 999;      // 1kHz/(999+1) = 1Hz (1秒中断)
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }
    
    // 配置TIM3中断
    HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

// PA12外部中断处理函数
void EXTI4_15_IRQHandler(void) {
    if (__HAL_GPIO_EXTI_GET_IT(PHOTO_SWITCH_PIN) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(PHOTO_SWITCH_PIN);
        
        // 简单防抖
        volatile uint32_t delay = DEBOUNCE_DELAY_COUNT;
        while(delay--);

			//speed_data.pulse_count++;
        // 确认电平
        if (HAL_GPIO_ReadPin(PHOTO_SWITCH_PORT, PHOTO_SWITCH_PIN) == GPIO_PIN_RESET) {
            speed_data.pulse_count++;
        }
    }
}

// 定时器3中断处理函数 - 1秒中断
void TIM3_IRQHandler(void) {
    if (__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_UPDATE) != RESET) {
        __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);

        // 设置测量完成标志
        speed_data.measurement_done = 1;
    }
}

void GDKGGetData(float* velocity_mps)
{
        if (speed_data.measurement_done) {
            speed_data.measurement_done = 0;

					  s_current_pulse_count = speed_data.pulse_count;  // 保存当前秒的脉冲计数
					
            // 计算速度
            // 速度(m/s) = 脉冲频率(Hz) × 0.05
            float pulse_freq_hz = (float)speed_data.pulse_count;
            speed_data.velocity_mps = pulse_freq_hz * PULSE_TO_VELOCITY_FACTOR;
            // speed_data.velocity_kmph = speed_data.velocity_mps * 3.6f;
            
            // 计算风速等级
            float v = speed_data.velocity_mps;
            if (v <= 0.2f) speed_data.wind_grade = 0;
            else if (v <= 1.5f) speed_data.wind_grade = 1;
            else if (v <= 3.3f) speed_data.wind_grade = 2;
            else if (v <= 5.4f) speed_data.wind_grade = 3;
            else if (v <= 7.9f) speed_data.wind_grade = 4;
            else if (v <= 10.7f) speed_data.wind_grade = 5;
            else if (v <= 13.8f) speed_data.wind_grade = 6;
            else if (v <= 17.1f) speed_data.wind_grade = 7;
            else if (v <= 20.7f) speed_data.wind_grade = 8;
            else if (v <= 24.4f) speed_data.wind_grade = 9;
            else if (v <= 28.4f) speed_data.wind_grade = 10;
            else if (v <= 32.6f) speed_data.wind_grade = 11;
            else speed_data.wind_grade = 12;
            
					speed_data.pulse_count = 0;  // 重置脉冲计数器
					
            // 保存并重置脉冲计数
        if (velocity_mps != NULL) {
				*velocity_mps = speed_data.velocity_mps;
				                           }
                       }
}

void I2C_SendGDKGData(void)
{
    static uint32_t last_send_time = 0;
    uint32_t current_time = HAL_GetTick();
    
    // 每秒发送一次数据
    if (current_time - last_send_time >= 1000) {
        last_send_time = current_time;
        
        
			 if (!I2C_For_GDKG_IsInitialized()) {
            if (I2C_For_GDKG_Init() != HAL_OK) {
                return;  // 初始化失败，不发送数据
            }
        }
        
        // 检查设备是否存在
        if (I2C_For_GDKG_CheckDevice() != HAL_OK) {
            return;  // 设备不存在，不发送数据
        }
			
        // 准备发送数据
        uint8_t tx_buffer[10];
        uint8_t index = 0;
        
        // 状态寄存器
        tx_buffer[index++] = GDKG_STATUS_READY;
        
        // 速度(m/s) - 乘以100转换为整数
        uint16_t velocity_mps_int = (uint16_t)(speed_data.velocity_mps * 100.0f);
        tx_buffer[index++] = (uint8_t)((velocity_mps_int >> 8) & 0xFF);
        tx_buffer[index++] = (uint8_t)(velocity_mps_int & 0xFF);
        
        // 速度(km/h) - 乘以100转换为整数
        uint16_t velocity_kmph_int = (uint16_t)(speed_data.velocity_kmph * 100.0f);
        tx_buffer[index++] = (uint8_t)((velocity_kmph_int >> 8) & 0xFF);
        tx_buffer[index++] = (uint8_t)(velocity_kmph_int & 0xFF);
        
        // 脉冲计数
        uint32_t pulse_count = s_current_pulse_count;
        tx_buffer[index++] = (uint8_t)((pulse_count >> 24) & 0xFF);
        tx_buffer[index++] = (uint8_t)((pulse_count >> 16) & 0xFF);
        tx_buffer[index++] = (uint8_t)((pulse_count >> 8) & 0xFF);
        tx_buffer[index++] = (uint8_t)(pulse_count & 0xFF);
        
        // 设备ID
        tx_buffer[index++] = 0x47; // 'G'的ASCII码
        
        // 发送数据
        HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c_for_gdkg,   I2C_FOR_GDKG_ADDRESS << 1,tx_buffer, index, 1000);
        
				        // 发送成功后可以重置脉冲计数
        if (status == HAL_OK) {
            // 可选：发送成功后重置脉冲计数
            speed_data.pulse_count = 0;
        }
				
    }
}

// I2C数据接收函数 - 从I2C读取配置
void I2C_ReceiveGDKGConfig(void)
{
    static uint32_t last_receive_time = 0;
    uint32_t current_time = HAL_GetTick();
    
    // 每5秒检查一次配置
    if (current_time - last_receive_time >= 5000) {
        last_receive_time = current_time;
        
        // 检查I2C是否已初始化
        if (!I2C_For_GDKG_IsInitialized()) {
            return;
        }
        
        // 读取控制寄存器
        uint8_t ctrl_reg = 0;
        if (I2C_For_GDKG_ReadRegister(GDKG_I2C_REG_CTRL, &ctrl_reg) == HAL_OK) {
            // 检查是否需要重置脉冲计数
            if (ctrl_reg & GDKG_CTRL_RESET_PULSE) {
                speed_data.pulse_count = 0;      // 重置当前脉冲计数
                s_current_pulse_count = 0;       // 重置当前秒的脉冲计数
                speed_data.velocity_mps = 0.0f;  // 重置速度
                speed_data.velocity_kmph = 0.0f; // 重置速度
                // 清除重置位
                I2C_For_GDKG_WriteRegister(GDKG_I2C_REG_CTRL, ctrl_reg & ~GDKG_CTRL_RESET_PULSE);
            }
        }
    }
}

#ifndef __GDKG_H
#define __GDKG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "py32f0xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
// ��翪�ض���
#define PHOTO_SWITCH_PIN     GPIO_PIN_12
#define PHOTO_SWITCH_PORT    GPIOA
#define PHOTO_SWITCH_IRQn    EXTI4_15_IRQn    

// ���ٲ���
#define MEASUREMENT_PERIOD_MS     1000    // 1���������
#define PULSE_TO_VELOCITY_FACTOR  0.08835f   // �ٶ�ת��ϵ����0.05 ��/�� ÿHz
#define DEBOUNCE_DELAY_COUNT      100    // ������ʱ����

// �������ݽṹ
typedef struct {
    volatile uint32_t pulse_count;        // ��ǰ�������
    uint32_t last_pulse_count;            // �ϴ��������
    float velocity_mps;                   // �ٶȣ���/��
    float velocity_kmph;                  // �ٶȣ�ǧ��/Сʱ
    uint8_t wind_grade;                   // ���ٵȼ� (0-12)
    uint8_t measurement_done;             // ������ɱ�־
} SpeedMeasurement_t;

// ȫ�ֱ�������
extern SpeedMeasurement_t speed_data;
extern TIM_HandleTypeDef htim3;
extern SPI_HandleTypeDef Spi1Handle;
extern uint8_t TxBuff[2];
extern uint8_t RxBuff[2];
extern uint16_t SC60228Data;

// ��������
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_TIM3_Init(void);
void Error_Handler(void);

// ��翪�ز�����غ���
void PhotoSwitch_Init(void);
void TIM3_Init(void);
void UART_Printf(const char *fmt, ...);
void GDKGGetData(float* velocity_mps);

void IIC_Init(void);
void I2C_SendGDKGData(void);
void I2C_ReceiveGDKGConfig(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

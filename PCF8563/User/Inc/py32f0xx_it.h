/**
  ******************************************************************************
  * @file    p32f031_it.h
	* @author  Application Team
	* @Version V1.0.0
  * @Date    
  * @brief   This file contains the headers of the interrupt handlers.
  ******************************************************************************
  */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __P32F031_IT_H
#define __P32F031_IT_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void NMI_Handler(void);
void HardFault_Handler(void);
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);


void FLASH_IRQHandler(void);
void UART0_IRQHandler(void);
void UART1_IRQHandler(void);
void LPUART_IRQHandler(void);
void SPI_IRQHandler(void);
void I2C_IRQHandler(void);

void LPTIM_IRQHandler(void);
void TIM1_IRQHandler(void);

void WWDG_IRQHandler(void);
void IWDG_IRQHandler(void);
void ADC_IRQHandler(void);

void RTC_IRQHandler(void);


#ifdef __cplusplus
}
#endif

#endif /* __P32F031_IT_H */



#ifndef MAIN_H
#define MAIN_H

#include "stm32f1xx_hal.h"

// Khai báo các đối tượng ngoại vi (sẽ được định nghĩa trong main.c)
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;

// Nguyên mẫu các hàm khởi tạo (thường do CubeMX sinh ra)
void SystemClock_Config(void);
void Error_Handler(void);
void MX_GPIO_Init(void);
void MX_SPI1_Init(void);
void MX_USART1_UART_Init(void);

#endif /* MAIN_H */
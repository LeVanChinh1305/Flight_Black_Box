#include "main.h"
#include "BMI160.h"
#include <stdio.h>

// Handle các thiết bị
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;
bmi_dev_t bmi;

// Redirect printf qua UART
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 100);
    return len;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    
    // Khởi tạo ngoại vi
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();

    // Cấu hình BMI160 (Range và ODR)
    BMI160_Config_t conf = {
        .accel_range = BMI_ACC_RANGE_2G,
        .gyro_range  = BMI_GYR_RANGE_2000DPS,
        .accel_odr   = 0x28, // 100Hz
        .gyro_odr    = 0x28  // 100Hz
    };

    // Khởi tạo BMI160 (Thay GPIOA, GPIO_PIN_4 bằng chân CS thực tế của bạn)
    if (BMI160_Init(&bmi, &hspi1, GPIOA, GPIO_PIN_3, &conf) == HAL_OK) {
        printf("BMI160 Init OK!\r\n");
    } else {
        printf("BMI160 Init Failed!\r\n");
        Error_Handler();
    }

    BMI160_Data data;

    while (1) {
        if (BMI160_ReadData(&bmi, &data) == HAL_OK) {
            // In dữ liệu thô ra màn hình: ADC nội bộ của cảm biến 
            printf("ACC: X=%d Y=%d Z=%d | GYR: X=%d Y=%d Z=%d\r\n", 
                    data.acc_x, data.acc_y, data.acc_z, 
                    data.gyr_x, data.gyr_y, data.gyr_z);
        }
        
        HAL_Delay(100); // Đọc mỗi 100ms
    }
    
}
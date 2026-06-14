#ifndef BMI_PROCESSING_H
#define BMI_PROCESSING_H

/**
 * @file    BMI_Processing.h
 * @brief   Module tính toán dữ liệu thô BMI160 → đơn vị vật lý.
 *
 * Chức năng:
 *   - Nhận RawSensorData_t từ xQueueRaw
 *   - Quy đổi LSB → g, deg/s, °C
 *   - Tính góc pitch/roll từ accelerometer
 *   - Đẩy ProcessedData_t vào xQueueProcessed
 *
 * Sử dụng:
 *   1. Gọi BMI_Processing_Init() sau khi tạo Queue nhưng trước vTaskStartScheduler()
 *   2. Tạo task bằng BMI_Processing_TaskHandle() hoặc trực tiếp dùng BMI_Processing_Task
 */

#include "app_types.h"

// HỆ SỐ QUY ĐỔI — thay đổi ở đây nếu đổi Range cấu hình BMI160

// Gia tốc kế ±2g  → 16384 LSB/g  (Datasheet Table 3) 
#define BMI_PROC_ACC_SENS_2G        16384.0f
// Gia tốc kế ±4g  → 8192  LSB/g  
#define BMI_PROC_ACC_SENS_4G         8192.0f
// Gia tốc kế ±8g  → 4096  LSB/g
#define BMI_PROC_ACC_SENS_8G         4096.0f
// Gia tốc kế ±16g → 2048  LSB/g  
#define BMI_PROC_ACC_SENS_16G        2048.0f

/** Gyro ±2000 dps → 16.4  LSB/(deg/s)  (Datasheet Table 4) */
#define BMI_PROC_GYR_SENS_2000DPS      16.4f
/** Gyro ±1000 dps → 32.8  LSB/(deg/s) */
#define BMI_PROC_GYR_SENS_1000DPS      32.8f
/** Gyro ±500  dps → 65.6  LSB/(deg/s) */
#define BMI_PROC_GYR_SENS_500DPS       65.6f
/** Gyro ±250  dps → 131.2 LSB/(deg/s) */
#define BMI_PROC_GYR_SENS_250DPS      131.2f
/** Gyro ±125  dps → 262.4 LSB/(deg/s) */
#define BMI_PROC_GYR_SENS_125DPS      262.4f

// Chọn sensitivity đang dùng (phải khớp với cấu hình BMI160_Init trong main)
#define BMI_PROC_ACC_SENSITIVITY    BMI_PROC_ACC_SENS_2G
#define BMI_PROC_GYR_SENSITIVITY    BMI_PROC_GYR_SENS_2000DPS

// API

/**
 * @brief  Khởi tạo module (hiện tại chưa cần, để sẵn cho sau nếu thêm Kalman).
 * @retval 0 = OK
 */
int BMI_Processing_Init(void);

/**
 * @brief  FreeRTOS task function — truyền vào xTaskCreate().
 * @param  pvParameters  Không dùng, truyền NULL.
 */
void BMI_Processing_Task(void *pvParameters);

#endif /* BMI_PROCESSING_H */
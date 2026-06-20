// Định nghĩa các kiểu dữ liệu và Queue dùng chung toàn ứng dụng.
// Task_ReadBMI160  --[xQueueRaw]-->  Task_ProcessBMI  --[xQueueProcessed]-->  Task_SendUART
#ifndef APP_TYPES_H
#define APP_TYPES_H

// FreeRTOS.h và queue.h được include trong từng .c file cần dùng,
// không include ở đây để tránh lỗi include path khi build lib riêng lẻ.
#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

// Dữ liệu thô ADC đọc trực tiếp từ BMI160 — đầu ra Task_ReadBMI160
typedef struct {
    int16_t  acc_x;           // Gia tốc X thô [LSB]
    int16_t  acc_y;           // Gia tốc Y thô [LSB]
    int16_t  acc_z;           // Gia tốc Z thô [LSB]
    int16_t  gyr_x;           // Gyro X thô    [LSB]
    int16_t  gyr_y;           // Gyro Y thô    [LSB]
    int16_t  gyr_z;           // Gyro Z thô    [LSB]
    int16_t  temp;            // Nhiệt độ thô  [LSB]
    uint32_t timestamp_ms;    // Thời điểm đọc [ms từ khi boot]
} RawSensorData_t;

// Dữ liệu đã quy đổi sang đơn vị vật lý — đầu ra Task_ProcessBMI
typedef struct {
    float    acc_x_g;         // Gia tốc X  [g]
    float    acc_y_g;         // Gia tốc Y  [g]
    float    acc_z_g;         // Gia tốc Z  [g]
    float    gyr_x_dps;       // Góc quay X [deg/s]
    float    gyr_y_dps;       // Góc quay Y [deg/s]
    float    gyr_z_dps;       // Góc quay Z [deg/s]
    float    temp_c;          // Nhiệt độ   [°C]
    float    pitch_deg;       // Góc pitch  [°]
    float    roll_deg;        // Góc roll   [°]
    uint32_t timestamp_ms;
} ProcessedData_t;

// Queue handle (định nghĩa trong main.c, extern để các module dùng)
extern QueueHandle_t xQueueRaw;
extern QueueHandle_t xQueueProcessed;

#define QUEUE_RAW_SIZE       4
#define QUEUE_PROCESSED_SIZE 4

#endif /* APP_TYPES_H */
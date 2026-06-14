#include "BMI_Reader.h"
#include "task.h"
#include <stdio.h>
#include "FreeRTOS.h"



// Con trỏ tới device handle — được gắn qua BMI_Reader_Init() 
static bmi_dev_t *s_bmi_dev = NULL;

// API CÔNG KHAI

int BMI_Reader_Init(bmi_dev_t *dev) {
    if (dev == NULL) {
        printf("[BMI_READER] Init failed: dev is NULL\r\n");
        return -1;
    }
    s_bmi_dev = dev;
    printf("[BMI_READER] Init OK (period=%dms)\r\n", BMI_READER_PERIOD_MS);
    return 0;
}

void BMI_Reader_Task(void *pvParameters) {
    (void)pvParameters;

    BMI160_Data      sensor_data;
    RawSensorData_t  raw;
    TickType_t       xLastWakeTime  = xTaskGetTickCount();
    const TickType_t xPeriod        = pdMS_TO_TICKS(BMI_READER_PERIOD_MS);
    uint8_t          err_count      = 0;

    for (;;) {
        // Timing cứng 100 Hz — vTaskDelayUntil không bị drift dù task bị preempt
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        if (BMI160_ReadData(s_bmi_dev, &sensor_data) == HAL_OK) {
            err_count = 0;  // reset chuỗi lỗi

            // Đóng gói vào RawSensorData_t
            raw.acc_x        = sensor_data.acc_x;
            raw.acc_y        = sensor_data.acc_y;
            raw.acc_z        = sensor_data.acc_z;
            raw.gyr_x        = sensor_data.gyr_x;
            raw.gyr_y        = sensor_data.gyr_y;
            raw.gyr_z        = sensor_data.gyr_z;
            raw.temp         = sensor_data.temp;
            raw.timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

            // Push vào queue — không block, nếu đầy thì drop frame cũ nhất
            if (xQueueSend(xQueueRaw, &raw, 0) != pdTRUE) {
                RawSensorData_t dummy;
                xQueueReceive(xQueueRaw, &dummy, 0);
                xQueueSend(xQueueRaw, &raw, 0);
                printf("[BMI_READER] xQueueRaw full, dropped oldest frame\r\n");
            }

        } else {
            err_count++;
            if (err_count >= BMI_READER_ERR_THRESHOLD) {
                printf("[BMI_READER] %d consecutive read errors!\r\n", err_count);
                err_count = 0;  // reset để không spam log liên tục
            }
        }
    }
}
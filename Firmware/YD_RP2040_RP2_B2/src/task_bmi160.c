#include "task_bmi160.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include <stdio.h>

static BMI160_Physical_t g_bmi_data = {0};
static bool g_bmi_data_valid = false;

bool BMI160_GetLastData(BMI160_Physical_t *out) {
    if (out == NULL || g_mutex_bmi_data == NULL) {
        return false;
    }

    bool ok = false;
    if (xSemaphoreTake(g_mutex_bmi_data, pdMS_TO_TICKS(BMI160_MUTEX_WAIT_MS)) == pdTRUE) {
        if (g_bmi_data_valid) {
            *out = g_bmi_data;
            ok = true;
        }
        xSemaphoreGive(g_mutex_bmi_data);
    }
    return ok;
}

void TaskBMI160(void *pvParameters) {
    (void)pvParameters;
    printf("[TaskBMI160] Khoi tao BMI160...\n");

    bmi_dev_t bmi;
    BMI160_Config_t config = {
        .accel_range = BMI_ACC_RANGE_4G,
        .gyro_range = BMI_GYR_RANGE_500DPS,
        .accel_odr = BMI_ACC_CONFIG_DEFAULT,
        .gyro_odr = BMI_GYR_CONFIG_DEFAULT,
    };

    while (BMI160_Init(&bmi, BMI160_I2C_PORT, BMI160_I2C_ADDR, &config) != BMI_OK) {
        printf("[TaskBMI160] Khoi tao BMI160 that bai, thu lai...\n");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    BMI160_FIFO_Config_t fifo_cfg = {
        .acc_en = true,
        .gyr_en = true,
        .header_en = true,
        .time_en = false,
        .watermark = 0,
        .fifo_mode = BMI160_FIFO_MODE_OVERWRITE,
        .overrun_en = true,
    };
    while (BMI160_FIFO_Config(&bmi, &fifo_cfg) != BMI_OK) {
        printf("[TaskBMI160] Cau hinh FIFO that bai, thu lai...\n");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    uint8_t scratch_buf[BMI160_FIFO_SCRATCH_SIZE];
    BMI160_FIFO_Frame_t frames[BMI160_FIFO_MAX_FRAMES];
    BMI160_FIFO_Result_t result;
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        BMI_Status status = BMI160_FIFO_ReadAndParse(&bmi, scratch_buf, BMI160_FIFO_SCRATCH_SIZE,
                                                     frames, BMI160_FIFO_MAX_FRAMES, &result);

        if (status == BMI_OK && result.frame_count > 0) {
            BMI160_FIFO_Frame_t *last = &frames[result.frame_count - 1];
            BMI160_Physical_t phys;
            if (BMI160_Algo_ConvertFrame(last, bmi.config.accel_range,
                                         bmi.config.gyro_range, &phys) == BMI_OK) {
                if (xSemaphoreTake(g_mutex_bmi_data, pdMS_TO_TICKS(BMI160_MUTEX_WAIT_MS)) == pdTRUE) {
                    g_bmi_data = phys;
                    g_bmi_data_valid = true;
                    xSemaphoreGive(g_mutex_bmi_data);
                }
            }
        } else if (status != BMI_OK) {
            printf("[TaskBMI160] Loi doc FIFO!\n");
        }
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(BMI160_READ_PERIOD_MS));
    }
}

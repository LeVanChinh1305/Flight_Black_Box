#include "task_bmi160.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#include "shared_data.h"

static BMI160_Physical_t g_bmi_data = {0};
static bool g_bmi_data_valid = false;

// ========== Event Detection Thresholds ==========
#define IMPACT_THRESHOLD_G          3.5f    // Va chạm: gia tốc vượt 3.5g tức thời
#define UNSTABLE_THRESHOLD_DPS      50.0f   // Mất ổn định: góc quay > 50 dps
#define UNSTABLE_DURATION_CYCLES    5       // Liên tục vượt ngưỡng trong 5 chu kỳ (1 giây)
#define VIBRATION_THRESHOLD_G       0.3f    // Rung động: biên độ > 0.3g xung quanh 1g
#define VIBRATION_WINDOW_CYCLES     10      // Cửa sổ trượt 2 giây (10 * 200ms)

// ========== Local Tracking ==========
static uint8_t g_unstable_counter = 0;     // Đếm số chu kỳ liên tục vượt ngưỡng unstable
static float g_prev_acc_mag = 0.0f;        // Gia tốc magnitude chu kỳ trước (để detect vibration)
static uint8_t g_vibration_count = 0;      // Đếm số lần dao động qua lại

static void check_and_publish_event(const BMI160_Physical_t *phys) {
    if (!phys || !g_mqtt_event_queue) {
        return;
    }

    // ========== 1. IMPACT Detection ==========
    if (phys->acc_magnitude_g > IMPACT_THRESHOLD_G) {
        mqtt_event_t evt = {0};
        evt.type = 0;  // event type
        snprintf(evt.payload, sizeof(evt.payload),
                 "{\"ts\":%u,\"type\":\"impact\",\"severity\":\"high\","
                 "\"acc_peak_g\":%.2f,\"gyr_peak_dps\":%.1f,\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f}",
                 xTaskGetTickCount() / 1000,
                 phys->acc_magnitude_g,
                 (phys->gyr_x_dps * phys->gyr_x_dps +
                  phys->gyr_y_dps * phys->gyr_y_dps +
                  phys->gyr_z_dps * phys->gyr_z_dps) > 0.0f ? 
                 __builtin_sqrtf(phys->gyr_x_dps * phys->gyr_x_dps +
                                phys->gyr_y_dps * phys->gyr_y_dps +
                                phys->gyr_z_dps * phys->gyr_z_dps) : 0.0f,
                 0.0f, 0.0f, 0.0f);  // TODO: Thêm GPS data
        xQueueSend(g_mqtt_event_queue, &evt, 0);
        printf("[TaskBMI160] IMPACT detected: acc_mag=%.2fg\n", phys->acc_magnitude_g);
    }

    // ========== 2. UNSTABLE Detection ==========
    float gyr_magnitude = __builtin_sqrtf(phys->gyr_x_dps * phys->gyr_x_dps +
                                          phys->gyr_y_dps * phys->gyr_y_dps +
                                          phys->gyr_z_dps * phys->gyr_z_dps);
    if (gyr_magnitude > UNSTABLE_THRESHOLD_DPS) {
        g_unstable_counter++;
        if (g_unstable_counter >= UNSTABLE_DURATION_CYCLES) {
            mqtt_event_t evt = {0};
            evt.type = 0;  // event type
            snprintf(evt.payload, sizeof(evt.payload),
                     "{\"ts\":%u,\"type\":\"unstable\",\"severity\":\"medium\","
                     "\"gyr_peak_dps\":%.1f,\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f}",
                     xTaskGetTickCount() / 1000,
                     gyr_magnitude, 0.0f, 0.0f, 0.0f);  // TODO: Thêm GPS data
            xQueueSend(g_mqtt_event_queue, &evt, 0);
            printf("[TaskBMI160] UNSTABLE detected: gyr_mag=%.1f dps\n", gyr_magnitude);
            g_unstable_counter = 0;  // Reset counter sau khi publish
        }
    } else {
        g_unstable_counter = 0;  // Reset nếu dưới ngưỡng
    }

    // ========== 3. VIBRATION Detection ==========
    // Detect nếu biên độ dao động quanh 1g vượt VIBRATION_THRESHOLD
    float acc_delta = phys->acc_magnitude_g - g_prev_acc_mag;
    if (acc_delta < 0) acc_delta = -acc_delta;
    
    if (acc_delta > VIBRATION_THRESHOLD_G && phys->acc_magnitude_g < 1.5f) {
        g_vibration_count++;
    }
    g_prev_acc_mag = phys->acc_magnitude_g;

    if (g_vibration_count >= VIBRATION_WINDOW_CYCLES) {
        mqtt_event_t evt = {0};
        evt.type = 0;  // event type
        snprintf(evt.payload, sizeof(evt.payload),
                 "{\"ts\":%u,\"type\":\"vibration\",\"severity\":\"low\","
                 "\"acc_mag_g\":%.2f,\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f}",
                 xTaskGetTickCount() / 1000,
                 phys->acc_magnitude_g, 0.0f, 0.0f, 0.0f);  // TODO: Thêm GPS data
        xQueueSend(g_mqtt_event_queue, &evt, 0);
        printf("[TaskBMI160] VIBRATION detected\n");
        g_vibration_count = 0;  // Reset counter
    }
}

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
        record_error(ERROR_BMI160_INIT);
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
                
                // Check and publish events
                check_and_publish_event(&phys);
            }
        } else if (status != BMI_OK) {
            record_error(ERROR_BMI160_READ);
        }
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(BMI160_READ_PERIOD_MS));
    }
}

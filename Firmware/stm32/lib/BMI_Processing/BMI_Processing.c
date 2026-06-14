#include "BMI_Processing.h"
#include <math.h>
#include <stdio.h>

// ===========================================================================
// HÀM NỘI BỘ
// ===========================================================================

/**
 * @brief  Quy đổi một bộ RawSensorData_t → ProcessedData_t.
 *         Tách ra hàm riêng để dễ unit-test sau này.
 */
static void convert_raw_to_physical(const RawSensorData_t *raw, ProcessedData_t *out) {

    // --- Gia tốc kế: LSB → g ---
    out->acc_x_g = (float)raw->acc_x / BMI_PROC_ACC_SENSITIVITY;
    out->acc_y_g = (float)raw->acc_y / BMI_PROC_ACC_SENSITIVITY;
    out->acc_z_g = (float)raw->acc_z / BMI_PROC_ACC_SENSITIVITY;

    // --- Con quay hồi chuyển: LSB → deg/s ---
    out->gyr_x_dps = (float)raw->gyr_x / BMI_PROC_GYR_SENSITIVITY;
    out->gyr_y_dps = (float)raw->gyr_y / BMI_PROC_GYR_SENSITIVITY;
    out->gyr_z_dps = (float)raw->gyr_z / BMI_PROC_GYR_SENSITIVITY;

    // --- Nhiệt độ: T[°C] = raw / 512 + 23  (Datasheet 2.12.10) ---
    out->temp_c = ((float)raw->temp / 512.0f) + 23.0f;

    // --- Góc nghiêng từ accelerometer (static tilt estimation) ---
    // Pitch: quay quanh trục Y — phụ thuộc acc_x vs sqrt(acc_y² + acc_z²)
    out->pitch_deg = atan2f(out->acc_x_g,
                            sqrtf(out->acc_y_g * out->acc_y_g +
                                  out->acc_z_g * out->acc_z_g))
                     * (180.0f / 3.14159265f);

    // Roll: quay quanh trục X — phụ thuộc acc_y vs acc_z
    out->roll_deg = atan2f(out->acc_y_g, out->acc_z_g)
                    * (180.0f / 3.14159265f);

    // --- Kế thừa timestamp ---
    out->timestamp_ms = raw->timestamp_ms;
}

// ===========================================================================
// API CÔNG KHAI
// ===========================================================================

int BMI_Processing_Init(void) {
    // Hiện tại chưa cần khởi tạo gì.
    // Chỗ này sẽ là nơi init Kalman filter, complementary filter, v.v.
    return 0;
}

void BMI_Processing_Task(void *pvParameters) {
    (void)pvParameters;

    RawSensorData_t raw;
    ProcessedData_t proc;

    for (;;) {
        // Block vô thời hạn, chờ Task_ReadBMI160 gửi dữ liệu vào queue
        if (xQueueReceive(xQueueRaw, &raw, portMAX_DELAY) == pdTRUE) {

            convert_raw_to_physical(&raw, &proc);

            // Đẩy sang Task_SendUART — nếu queue đầy thì drop frame cũ nhất
            if (xQueueSend(xQueueProcessed, &proc, 0) != pdTRUE) {
                ProcessedData_t dummy;
                xQueueReceive(xQueueProcessed, &dummy, 0);
                xQueueSend(xQueueProcessed, &proc, 0);
                printf("[PROC] xQueueProcessed full, dropped oldest frame\r\n");
            }
        }
    }
}
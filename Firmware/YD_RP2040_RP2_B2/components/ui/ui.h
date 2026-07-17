#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "semphr.h"

// ================   Cấu trúc tham số cho TaskUI   ==============
// Chứa con trỏ tới các Mutex dùng chung, được truyền vào lúc tạo Task.
typedef struct {
    SemaphoreHandle_t mutex_spi0; // Mutex bảo vệ bus SPI0 (SD Card + XPT2046)
} UI_TaskParams_t;

// ================   Khai báo getter   ==============
// TaskUI cần đọc dữ liệu GPS và BMI160 từ các Task khác thông qua 2 hàm này.
// Định nghĩa thực tế nằm trong main.cpp, khai báo extern để ui.cpp có thể gọi.

// Lấy dữ liệu GPS mới nhất (thread-safe, có mutex bên trong)
// Trả về true nếu có dữ liệu, false nếu chưa có
#include "neo6m.h"
bool GPS_GetLastData(neo6m_data_t *out);

// Lấy dữ liệu BMI160 mới nhất (thread-safe, có mutex bên trong)
// Trả về true nếu có dữ liệu hợp lệ, false nếu chưa có
#include "algorithm_bmi160.h"
bool BMI160_GetLastData(BMI160_Physical_t *out);

// ================   Entry-point của Task   ==============
// Được gọi bởi xTaskCreate() trong main.cpp
// pvParameters phải là con trỏ tới UI_TaskParams_t đã được cấp phát tĩnh.
void TaskUI(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // UI_H

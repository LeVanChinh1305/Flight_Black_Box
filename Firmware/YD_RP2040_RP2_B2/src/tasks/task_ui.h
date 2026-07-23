#ifndef TASK_UI_H
#define TASK_UI_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "neo6m.h"
#include "algorithm_bmi160.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Thông số khi tạo TaskUI
typedef struct {
    SemaphoreHandle_t mutex_spi0; // Bảo vệ SPI0 dùng cho XPT2046
} UI_TaskParams_t;

bool GPS_GetLastData(neo6m_data_t *out);
bool BMI160_GetLastData(BMI160_Physical_t *out);

void TaskUI(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // TASK_UI_H

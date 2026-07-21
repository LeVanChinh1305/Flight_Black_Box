#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

extern SemaphoreHandle_t g_mutex_gps_data;
extern SemaphoreHandle_t g_mutex_bmi_data;
extern SemaphoreHandle_t g_mutex_spi0;
extern QueueHandle_t g_buzzer_queue;

#ifdef __cplusplus
}
#endif

#endif // SHARED_DATA_H

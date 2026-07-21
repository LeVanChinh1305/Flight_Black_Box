#ifndef TASK_GPS_H
#define TASK_GPS_H

#include "FreeRTOS.h"
#include "task.h"
#include "neo6m.h"
#include "shared_data.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PRIORITY_TASK_GPS (tskIDLE_PRIORITY + 2)
#define GPS_TASK_STACK_SIZE 1024
#define GPS_MUTEX_WAIT_MS 50

void TaskGPS(void *pvParameters);
bool GPS_GetLastData(neo6m_data_t *out);

#ifdef __cplusplus
}
#endif

#endif // TASK_GPS_H

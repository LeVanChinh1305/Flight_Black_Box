#ifndef TASK_MQTT_H
#define TASK_MQTT_H

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PRIORITY_TASK_MQTT (tskIDLE_PRIORITY + 1)
#define MQTT_TASK_STACK_SIZE 3072

void TaskMQTT(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // TASK_MQTT_H

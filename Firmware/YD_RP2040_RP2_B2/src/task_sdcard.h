#ifndef TASK_SDCARD_H
#define TASK_SDCARD_H

#include "FreeRTOS.h"
#include "task.h"
#include "sdcard.h"
#include "neo6m.h"
#include "algorithm_bmi160.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PRIORITY_TASK_SDCARD (tskIDLE_PRIORITY + 1)
#define SDCARD_TASK_STACK_SIZE 2048
#define SDCARD_WRITE_PERIOD_MS 1000
#define SDCARD_ROTATE_EVERY_N_CYCLES 60
#define SDCARD_MOUNT_RETRY_MS 1000
#define SDLOG_FILENAME_MAXLEN 13

void TaskSDCard(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // TASK_SDCARD_H

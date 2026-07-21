#ifndef TASK_BMI160_H
#define TASK_BMI160_H

#include "FreeRTOS.h"
#include "task.h"
#include "algorithm_bmi160.h"
#include "shared_data.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PRIORITY_TASK_BMI160 (tskIDLE_PRIORITY + 3)
#define BMI160_TASK_STACK_SIZE 1024
#define BMI160_READ_PERIOD_MS 200
#define BMI160_FIFO_SCRATCH_SIZE 256
#define BMI160_FIFO_MAX_FRAMES 32
#define BMI160_MUTEX_WAIT_MS 50

void TaskBMI160(void *pvParameters);
bool BMI160_GetLastData(BMI160_Physical_t *out);

#ifdef __cplusplus
}
#endif

#endif // TASK_BMI160_H

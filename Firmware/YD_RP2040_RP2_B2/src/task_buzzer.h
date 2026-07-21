#ifndef TASK_BUZZER_H
#define TASK_BUZZER_H

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PRIORITY_TASK_BUZZER (tskIDLE_PRIORITY + 1)
#define BUZZER_TASK_STACK_SIZE 512
#define BUZZER_QUEUE_LEN 8

typedef enum {
    BUZZER_CMD_BOOT_OK = 0,
    BUZZER_CMD_BOOT_ERROR = 1,
    BUZZER_CMD_BEEP_SHORT = 2,
    BUZZER_CMD_BEEP_LONG = 3,
    BUZZER_CMD_ALARM = 4,
    BUZZER_CMD_ALARM_STOP = 5,
} BuzzerCmd_t;

#define BUZZER_BEEP_SHORT_MS 100
#define BUZZER_BEEP_LONG_MS 500

bool Buzzer_SendCmd(BuzzerCmd_t cmd);
void TaskBuzzer(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // TASK_BUZZER_H

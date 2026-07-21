#include "task_buzzer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include <stdio.h>

#include "buzzer.h"
#include "shared_data.h"

bool Buzzer_SendCmd(BuzzerCmd_t cmd) {
    if (g_buzzer_queue == NULL) {
        return false;
    }
    return xQueueSend(g_buzzer_queue, &cmd, 0) == pdTRUE;
}

void TaskBuzzer(void *pvParameters) {
    (void)pvParameters;

    Buzzer_Init();
    printf("[TaskBuzzer] Khoi tao buzzer GPIO %d thanh cong.\n", BUZZER_PIN);

    gpio_put(BUZZER_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(BUZZER_BOOT_OK_BEEP_MS));
    gpio_put(BUZZER_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(BUZZER_BOOT_OK_GAP_MS));
    gpio_put(BUZZER_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(BUZZER_BOOT_OK_BEEP_MS));
    gpio_put(BUZZER_PIN, 0);
    printf("[TaskBuzzer] Boot OK beep phat xong.\n");

    bool alarm_active = false;
    BuzzerCmd_t cmd;
    for (;;) {
        TickType_t wait = alarm_active ? pdMS_TO_TICKS(BUZZER_ALARM_PERIOD_MS) : portMAX_DELAY;

        if (xQueueReceive(g_buzzer_queue, &cmd, wait) == pdTRUE) {
            switch (cmd) {
                case BUZZER_CMD_BOOT_OK:
                    gpio_put(BUZZER_PIN, 1);
                    vTaskDelay(pdMS_TO_TICKS(BUZZER_BOOT_OK_BEEP_MS));
                    gpio_put(BUZZER_PIN, 0);
                    vTaskDelay(pdMS_TO_TICKS(BUZZER_BOOT_OK_GAP_MS));
                    gpio_put(BUZZER_PIN, 1);
                    vTaskDelay(pdMS_TO_TICKS(BUZZER_BOOT_OK_BEEP_MS));
                    gpio_put(BUZZER_PIN, 0);
                    break;
                case BUZZER_CMD_BOOT_ERROR:
                    gpio_put(BUZZER_PIN, 1);
                    vTaskDelay(pdMS_TO_TICKS(BUZZER_ERROR_BEEP_MS));
                    gpio_put(BUZZER_PIN, 0);
                    break;
                case BUZZER_CMD_BEEP_SHORT:
                    gpio_put(BUZZER_PIN, 1);
                    vTaskDelay(pdMS_TO_TICKS(BUZZER_BEEP_SHORT_MS));
                    gpio_put(BUZZER_PIN, 0);
                    break;
                case BUZZER_CMD_BEEP_LONG:
                    gpio_put(BUZZER_PIN, 1);
                    vTaskDelay(pdMS_TO_TICKS(BUZZER_BEEP_LONG_MS));
                    gpio_put(BUZZER_PIN, 0);
                    break;
                case BUZZER_CMD_ALARM:
                    alarm_active = true;
                    printf("[TaskBuzzer] Che do ALARM bat.\n");
                    break;
                case BUZZER_CMD_ALARM_STOP:
                    alarm_active = false;
                    gpio_put(BUZZER_PIN, 0);
                    printf("[TaskBuzzer] Che do ALARM tat.\n");
                    break;
                default:
                    break;
            }
        } else {
            if (alarm_active) {
                gpio_put(BUZZER_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(BUZZER_ALARM_BEEP_MS));
                gpio_put(BUZZER_PIN, 0);
            }
        }
    }
}

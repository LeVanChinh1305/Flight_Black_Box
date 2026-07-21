#include <cstdio>
#include "FreeRTOS.h"
#include "pico/stdlib.h"
#include "task.h"

#include "shared_data.h"
#include "task_bmi160.h"
#include "task_gps.h"
#include "task_sdcard.h"
#include "task_mqtt.h"
#include "task_buzzer.h"
#include "task_ui.h"

static UI_TaskParams_t g_ui_params;

SemaphoreHandle_t g_mutex_gps_data = NULL;
SemaphoreHandle_t g_mutex_bmi_data = NULL;
SemaphoreHandle_t g_mutex_spi0 = NULL;
QueueHandle_t g_buzzer_queue = NULL;

extern "C" void vApplicationMallocFailedHook(void) {
    printf("!!! LOI: FreeRTOS het HEAP (pvPortMalloc that bai)! Hay tang configTOTAL_HEAP_SIZE. !!!\n");
    taskDISABLE_INTERRUPTS();
    while (true) {
        tight_loop_contents();
    }
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    printf("!!! STACK OVERFLOW o task: %s !!!\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    while (true) {
        tight_loop_contents();
    }
}

int main() {
    stdio_init_all();
    sleep_ms(5000);

    g_mutex_gps_data = xSemaphoreCreateMutex();
    g_mutex_bmi_data = xSemaphoreCreateMutex();
    g_mutex_spi0 = xSemaphoreCreateMutex();
    g_buzzer_queue = xQueueCreate(BUZZER_QUEUE_LEN, sizeof(BuzzerCmd_t));
    if (g_mutex_gps_data == NULL || g_mutex_bmi_data == NULL || g_mutex_spi0 == NULL || g_buzzer_queue == NULL) {
        printf("[main] Khong the tao mutex/queue!\n");
        while (true) {
            tight_loop_contents();
        }
    }

    printf("=== Flight Black Box - Bat dau chuong trinh ===\n");

    BaseType_t ok_BMI160 = xTaskCreate(TaskBMI160, "TaskBMI160", BMI160_TASK_STACK_SIZE, NULL, PRIORITY_TASK_BMI160, NULL);
    BaseType_t ok_GPS = xTaskCreate(TaskGPS, "GPS", GPS_TASK_STACK_SIZE, NULL, PRIORITY_TASK_GPS, NULL);
    BaseType_t ok_SDCARD = xTaskCreate(TaskSDCard, "SDCARD", SDCARD_TASK_STACK_SIZE, NULL, PRIORITY_TASK_SDCARD, NULL);
    BaseType_t ok_MQTT = xTaskCreate(TaskMQTT, "MQTT", MQTT_TASK_STACK_SIZE, NULL, PRIORITY_TASK_MQTT, NULL);
    BaseType_t ok_BUZZER = xTaskCreate(TaskBuzzer, "BUZZER", BUZZER_TASK_STACK_SIZE, NULL, PRIORITY_TASK_BUZZER, NULL);

    g_ui_params.mutex_spi0 = g_mutex_spi0;
    BaseType_t ok_UI = xTaskCreate(TaskUI, "UI", 2048, &g_ui_params, (tskIDLE_PRIORITY + 1), NULL);

    if (ok_BMI160 != pdPASS || ok_GPS != pdPASS || ok_SDCARD != pdPASS || ok_UI != pdPASS || ok_BUZZER != pdPASS || ok_MQTT != pdPASS) {
        printf("Loi: khong the tao task!\n");
        while (true) {
            tight_loop_contents();
        }
    }

    vTaskStartScheduler();

    while (true) {
        tight_loop_contents();
    }
    return 0;
}

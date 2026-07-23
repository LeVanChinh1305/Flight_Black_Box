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

static UI_TaskParams_t g_ui_params;         // Tham số khi tạo TaskUI (chứa mutex_spi0)

SemaphoreHandle_t g_mutex_gps_data = NULL;  // Bảo vệ truy cập dữ liệu GPS (g_gps_data)
SemaphoreHandle_t g_mutex_bmi_data = NULL;  // Bảo vệ truy cập dữ liệu BMI160 (g_bmi_data)
SemaphoreHandle_t g_mutex_spi0 = NULL;      // Bảo vệ truy cập SPI0 
QueueHandle_t g_buzzer_queue = NULL;        // Queue cho lệnh điều khiển buzzer từ các task khác → TaskBuzzer
QueueHandle_t g_mqtt_event_queue = NULL;    // Queue cho sự kiện event/card từ TaskBMI160, TaskSDCard → TaskMQTT
SemaphoreHandle_t g_mutex_error_list = NULL;// Bảo vệ truy cập danh sách lỗi (g_error_list)
error_list_t g_error_list = {0};            // Danh sách lỗi hiện tại (mã lỗi, số lượng lỗi)


// ================== FreeRTOS Hooks ==================
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
    stdio_init_all(); // UART0 (USB) cho printf, scanf, v.v.
    sleep_ms(5000);   // Delay 5s để chờ USB Serial kết nối (nếu có) trước khi printf log

    //khởi tạo mutex bảo vệ dữ liệu dùng chung và queue cho các task
    g_mutex_gps_data = xSemaphoreCreateMutex();
    g_mutex_bmi_data = xSemaphoreCreateMutex();
    g_mutex_spi0 = xSemaphoreCreateMutex();
    g_mutex_error_list = xSemaphoreCreateMutex();

    g_buzzer_queue = xQueueCreate(BUZZER_QUEUE_LEN, sizeof(BuzzerCmd_t));
    g_mqtt_event_queue = xQueueCreate(8, sizeof(mqtt_event_t));  // Queue cho sự kiện MQTT (impact, card events, v.v.)



    if (g_mutex_gps_data == NULL || g_mutex_bmi_data == NULL || g_mutex_spi0 == NULL || 
        g_mutex_error_list == NULL || g_buzzer_queue == NULL || g_mqtt_event_queue == NULL) {
        printf("[main] Khong the tao mutex/queue!\n");
        while (true) {
            tight_loop_contents();
        }
    }

    printf("=== Flight Black Box - Bat dau chuong trinh ===\n");

    // Tạo các task chính
    BaseType_t ok_BMI160 =  xTaskCreate(TaskBMI160,     "TaskBMI160",   BMI160_TASK_STACK_SIZE, NULL, PRIORITY_TASK_BMI160, NULL);
    BaseType_t ok_GPS    =  xTaskCreate(TaskGPS,        "GPS",          GPS_TASK_STACK_SIZE,    NULL, PRIORITY_TASK_GPS,    NULL);
    BaseType_t ok_SDCARD =  xTaskCreate(TaskSDCard,     "SDCARD",       SDCARD_TASK_STACK_SIZE, NULL, PRIORITY_TASK_SDCARD, NULL);
    BaseType_t ok_MQTT   =  xTaskCreate(TaskMQTT,       "MQTT",         MQTT_TASK_STACK_SIZE,   NULL, PRIORITY_TASK_MQTT,   NULL);
    BaseType_t ok_BUZZER =  xTaskCreate(TaskBuzzer,     "BUZZER",       BUZZER_TASK_STACK_SIZE, NULL, PRIORITY_TASK_BUZZER, NULL);

    g_ui_params.mutex_spi0 = g_mutex_spi0;// Truyền mutex_spi0 cho TaskUI để UI có thể dùng SPI0 (OLED, SDCard) mà không xung đột với các task khác
    BaseType_t ok_UI     =  xTaskCreate(TaskUI, "UI", 2048, &g_ui_params, (tskIDLE_PRIORITY + 1), NULL); 

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

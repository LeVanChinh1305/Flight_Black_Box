#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

// ========== Mutexes & Semaphores ==========
extern SemaphoreHandle_t g_mutex_gps_data;
extern SemaphoreHandle_t g_mutex_bmi_data;
extern SemaphoreHandle_t g_mutex_spi0;

// ========== Command Queues ==========
extern QueueHandle_t g_buzzer_queue;

// ========== Event Queue cho MQTT ==========
// Sử dụng để TaskBMI160 và TaskSDCard gửi sự kiện sang TaskMQTT
// Cấu trúc:
//   - type: 0=event (impact/unstable/vibration), 1=card_event (start/periodic/stop/near_full/full)
//   - payload: JSON string (tối đa 256 bytes) chứa thông tin chi tiết
typedef struct {
    uint8_t type;           // 0: event, 1: card_event
    char payload[256];      // JSON payload
} mqtt_event_t;

extern QueueHandle_t g_mqtt_event_queue;

// ========== Error Tracking ==========
// Định nghĩa mã lỗi cho các thành phần khác nhau
#define ERROR_BMI160_READ       0x01
#define ERROR_BMI160_INIT       0x02
#define ERROR_GPS_NO_FIX        0x10
#define ERROR_GPS_READ          0x11
#define ERROR_SDCARD_MOUNT      0x20
#define ERROR_SDCARD_WRITE      0x21
#define ERROR_SDCARD_FULL       0x22
#define ERROR_MQTT_DISCONNECT   0x30
#define ERROR_MQTT_PUBLISH      0x31
#define ERROR_SIM7680_INIT      0x40
#define ERROR_SIM7680_CONN      0x41

// Error tracking structure
#define MAX_ERRORS 10
typedef struct {
    uint8_t error_codes[MAX_ERRORS];
    uint8_t error_count;
} error_list_t;

extern error_list_t g_error_list;
extern SemaphoreHandle_t g_mutex_error_list;

// Hàm thêm lỗi vào danh sách
void record_error(uint8_t error_code);
#ifdef __cplusplus
}
#endif

#endif // SHARED_DATA_H
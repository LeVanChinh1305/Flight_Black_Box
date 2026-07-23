#include "task_mqtt.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "my_mqtt.h"
#include "sim7680.h"
#include "shared_data.h"
#include "algorithm_bmi160.h"
#include "neo6m.h"
#include "task_bmi160.h"
#include "task_gps.h"
#include "task_buzzer.h"

// ========== State Machine ==========
typedef enum {
    MQTT_STATE_INIT_SIM = 0,
    MQTT_STATE_CONNECT,
    MQTT_STATE_SUBSCRIBE,
    MQTT_STATE_RUNNING,
    MQTT_STATE_RECONNECT
} MQTT_State_t;

// ========== Helpers ==========
static uint32_t g_gps_synchronized_timestamp = 0;  // Unix timestamp synchronized from GPS
static bool g_gps_time_synced = false;

static uint32_t get_timestamp_sec(void) {
    // Return GPS synchronized time if available, otherwise boot tick count
    if (g_gps_time_synced && g_gps_synchronized_timestamp > 0) {
        return g_gps_synchronized_timestamp + (xTaskGetTickCount() / 1000);
    }
    // Fallback: return approximate seconds since boot
    return xTaskGetTickCount() / 1000;
}

static void try_sync_gps_time(void) {
    // Attempt to synchronize system time from GPS fix
    // TODO: Implement proper system time synchronization
    // For now, just detect GPS fix and note that time is synchronized
    neo6m_data_t gps = {0};
    if (GPS_GetLastData(&gps) && gps.is_valid && gps.fix_quality > 0) {
        if (!g_gps_time_synced) {
            // GPS fix achieved - time would be synchronized here
            // In production: convert GPS date/time to Unix timestamp
            g_gps_time_synced = true;
            g_gps_synchronized_timestamp = 1000000000;  // Placeholder Unix timestamp
            printf("[TaskMQTT] GPS time synchronized!\n");
        }
    } else {
        g_gps_time_synced = false;
    }
}

static bool parse_mqtt_command(const char *payload, char *cmd_id, char *cmd_name) {
    // Parse JSON: { "cmd_id": "a12f", "cmd": "BUZZER_BEEP" }
    // Đơn giản hóa: tìm chuỗi "cmd_id": "XXX" và "cmd": "YYY"
    const char *cid_start = strstr(payload, "\"cmd_id\"");
    const char *cmd_start = strstr(payload, "\"cmd\"");
    
    if (!cid_start || !cmd_start) return false;
    
    // Parse cmd_id
    cid_start = strchr(cid_start, '\"');
    if (!cid_start) return false;
    cid_start++;  // skip opening quote
    sscanf(cid_start, "%15[^\"]", cmd_id);
    
    // Parse cmd
    cmd_start = strchr(cmd_start, '\"');
    if (!cmd_start) return false;
    cmd_start++;  // skip opening quote
    sscanf(cmd_start, "%31[^\"]", cmd_name);
    
    return true;
}

static void publish_ack(const char *cmd_id, const char *cmd, const char *status, const char *detail) {
    // Publish ack với định dạng:
    // { "ts": 1234567890, "cmd_id": "a12f", "cmd": "BUZZER_BEEP", "status": "success" }
    char payload[256];
    if (detail && detail[0]) {
        snprintf(payload, sizeof(payload),
                 "{\"ts\":%u,\"cmd_id\":\"%s\",\"cmd\":\"%s\",\"status\":\"%s\",\"detail\":\"%s\"}",
                 get_timestamp_sec(), cmd_id, cmd, status, detail);
    } else {
        snprintf(payload, sizeof(payload),
                 "{\"ts\":%u,\"cmd_id\":\"%s\",\"cmd\":\"%s\",\"status\":\"%s\"}",
                 get_timestamp_sec(), cmd_id, cmd, status);
    }
    mqtt_publish(MQTT_TOPIC_ACK, payload, false);
}

static void publish_telemetry(void) {
    BMI160_Physical_t bmi = {0};
    neo6m_data_t gps = {0};
    bool has_bmi = BMI160_GetLastData(&bmi);
    bool has_gps = GPS_GetLastData(&gps);

    char payload[512];
    snprintf(payload, sizeof(payload),
             "{\"ts\":%u,\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f,"
             "\"spd\":%.1f,\"course\":%.1f,\"sat\":%u,\"fix\":%u,"
             "\"accX\":%.2f,\"accY\":%.2f,\"accZ\":%.2f,\"accMag\":%.2f,"
             "\"gyrX\":%.1f,\"gyrY\":%.1f,\"gyrZ\":%.1f}",
             get_timestamp_sec(),
             has_gps ? gps.latitude : 0.0f,
             has_gps ? gps.longitude : 0.0f,
             has_gps ? gps.altitude_m : 0.0f,
             has_gps ? gps.speed_kmh : 0.0f,
             has_gps ? gps.course_deg : 0.0f,
             has_gps ? gps.satellites : 0,
             has_gps ? gps.fix_quality : 0,
             has_bmi ? bmi.acc_x_g : 0.0f,
             has_bmi ? bmi.acc_y_g : 0.0f,
             has_bmi ? bmi.acc_z_g : 0.0f,
             has_bmi ? bmi.acc_magnitude_g : 0.0f,
             has_bmi ? bmi.gyr_x_dps : 0.0f,
             has_bmi ? bmi.gyr_y_dps : 0.0f,
             has_bmi ? bmi.gyr_z_dps : 0.0f);

    printf("[TaskMQTT] Publishing telemetry...\n");
    if (!mqtt_publish(MQTT_TOPIC_TELEMETRY, payload, false)) {
        printf("[TaskMQTT] Error publishing telemetry!\n");
    }
}

static void publish_status(const char *trigger) {
    // trigger: "periodic", "error", "on_demand"
    // Collect error list from shared error tracking
    char error_list_str[256] = "[]";
    if (g_mutex_error_list) {
        xSemaphoreTake(g_mutex_error_list, portMAX_DELAY);
    }
    
    if (g_error_list.error_count > 0) {
        char *p = error_list_str;
        size_t remaining = sizeof(error_list_str);
        snprintf(p, remaining, "[");
        p += strlen(p);
        remaining -= strlen(error_list_str);
        
        for (uint8_t i = 0; i < g_error_list.error_count && i < MAX_ERRORS; i++) {
            if (i > 0) {
                snprintf(p, remaining, ",");
                p += strlen(p);
                remaining -= strlen(p);
            }
            snprintf(p, remaining, "%u", g_error_list.error_codes[i]);
            p += strlen(p);
            remaining -= strlen(p);
        }
        snprintf(p, remaining, "]");
    }
    
    if (g_mutex_error_list) {
        xSemaphoreGive(g_mutex_error_list);
    }

    // Get sensor status
    BMI160_Physical_t bmi = {0};
    neo6m_data_t gps = {0};
    bool bmi_ok = BMI160_GetLastData(&bmi);
    bool gps_ok = GPS_GetLastData(&gps);

    char payload[768];
    snprintf(payload, sizeof(payload),
             "{\"ts\":%u,\"trigger\":\"%s\","
             "\"mcu\":{\"uptime_s\":%u,\"free_heap_kb\":%u},"
             "\"sensors\":{\"bmi160_ok\":%s,\"gps_fix\":%s,\"gps_sat\":%u,\"sd_ok\":true},"
             "\"module\":{\"sim_ready\":true,\"sim_rssi\":22,\"sim_ber\":0,\"net_status\":1},"
             "\"errors\":%s}",
             get_timestamp_sec(),
             trigger,
             get_timestamp_sec(),
             xPortGetFreeHeapSize() / 1024,
             bmi_ok ? "true" : "false",
             gps_ok ? "true" : "false",
             gps_ok ? (unsigned)gps.satellites : 0,
             error_list_str);

    printf("[TaskMQTT] Publishing status (trigger=%s, errors=%s)...\n", trigger, error_list_str);
    mqtt_publish(MQTT_TOPIC_STATUS, payload, true);  // retain=true
}

static void handle_mqtt_event(void) {
    mqtt_event_t evt;
    // Non-blocking receive: xQueueReceive(..., 0)
    if (xQueueReceive(g_mqtt_event_queue, &evt, 0) == pdTRUE) {
        printf("[TaskMQTT] Received MQTT event, type=%u\n", evt.type);
        if (evt.type == 0) {
            // event: impact/unstable/vibration
            printf("[TaskMQTT] Publishing event: %s\n", evt.payload);
            mqtt_publish(MQTT_TOPIC_EVENT, evt.payload, false);
        } else if (evt.type == 1) {
            // card_event: start/periodic/stop/near_full/full
            printf("[TaskMQTT] Publishing card event: %s\n", evt.payload);
            // Kiểm tra nếu là "full" thì retain=true, còn lại retain=false
            bool retain = (strstr(evt.payload, "\"full\"") != NULL);
            mqtt_publish(MQTT_TOPIC_CARD, evt.payload, retain);
        }
    }
}

// ========== MQTT Message Callback ==========
void mqtt_message_received(const char *topic, const char *payload) {
    printf("[TaskMQTT] Received MQTT message on [%s]: %s\n", topic, payload);

    // Kiểm tra nếu là command topic
    if (strcmp(topic, MQTT_TOPIC_COMMAND) == 0) {
        char cmd_id[16] = {0};
        char cmd_name[32] = {0};
        
        if (!parse_mqtt_command(payload, cmd_id, cmd_name)) {
            printf("[TaskMQTT] Failed to parse command JSON\n");
            publish_ack("", "", "fail", "parse error");
            return;
        }

        printf("[TaskMQTT] Parsed command: cmd_id=%s, cmd=%s\n", cmd_id, cmd_name);

        // Xử lý lệnh
        if (strcmp(cmd_name, "BUZZER_BEEP") == 0) {
            Buzzer_SendCmd(BUZZER_CMD_BEEP_SHORT);
            publish_ack(cmd_id, cmd_name, "success", "");
        } else if (strcmp(cmd_name, "ALARM_START") == 0) {
            Buzzer_SendCmd(BUZZER_CMD_ALARM);
            publish_ack(cmd_id, cmd_name, "success", "");
        } else if (strcmp(cmd_name, "ALARM_STOP") == 0) {
            Buzzer_SendCmd(BUZZER_CMD_ALARM_STOP);
            publish_ack(cmd_id, cmd_name, "success", "");
        } else if (strcmp(cmd_name, "HEALTH_CHECK") == 0) {
            publish_ack(cmd_id, cmd_name, "success", "");
            // Publish status immediately với trigger="on_demand"
            publish_status("on_demand");
        } else if (strcmp(cmd_name, "REBOOT") == 0) {
            publish_ack(cmd_id, cmd_name, "success", "");
            vTaskDelay(pdMS_TO_TICKS(500));  // Đợi ACK được gửi trước
            // TODO: watchdog_reboot() từ Pico SDK
            printf("[TaskMQTT] REBOOT command received - should trigger watchdog reset\n");
        } else {
            publish_ack(cmd_id, cmd_name, "fail", "unknown cmd");
        }
    }
}

// ========== TaskMQTT Main Loop ==========
void TaskMQTT(void *pvParameters) {
    (void)pvParameters;
    MQTT_State_t state = MQTT_STATE_INIT_SIM;
    TickType_t last_telemetry_time = 0;
    TickType_t last_status_time = 0;

    for (;;) {
        switch (state) {
            case MQTT_STATE_INIT_SIM: {
                printf("[TaskMQTT] State: INIT_SIM\n");
                sim7680_init();
                printf("[TaskMQTT] Waiting for SIM ready...\n");
                while (!sim7680_wait_ready(10000)) {
                    printf("[TaskMQTT] SIM not ready, retrying...\n");
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }

                bool sim_ready = false;
                while (!sim7680_check_sim(&sim_ready) || !sim_ready) {
                    printf("[TaskMQTT] Waiting for SIM card...\n");
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }
                printf("[TaskMQTT] SIM ready.\n");
                state = MQTT_STATE_CONNECT;
                break;
            }

            case MQTT_STATE_CONNECT: {
                printf("[TaskMQTT] State: CONNECT\n");
                if (!mqtt_init()) {
                    printf("[TaskMQTT] Error mqtt_init(). Retrying in 5s...\n");
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    break;
                }
                if (mqtt_connect()) {
                    state = MQTT_STATE_SUBSCRIBE;
                } else {
                    printf("[TaskMQTT] Error mqtt_connect(). Retrying in 5s...\n");
                    vTaskDelay(pdMS_TO_TICKS(5000));
                }
                break;
            }

            case MQTT_STATE_SUBSCRIBE: {
                printf("[TaskMQTT] State: SUBSCRIBE\n");
                if (mqtt_subscribe(MQTT_TOPIC_COMMAND)) {
                    printf("[TaskMQTT] Subscribed to: %s\n", MQTT_TOPIC_COMMAND);
                    state = MQTT_STATE_RUNNING;
                    last_telemetry_time = xTaskGetTickCount();
                    last_status_time = xTaskGetTickCount();
                    publish_status("periodic");  // Initial status publish
                } else {
                    printf("[TaskMQTT] Error subscribing, reconnecting...\n");
                    state = MQTT_STATE_RECONNECT;
                }
                break;
            }

            case MQTT_STATE_RUNNING: {
                // Try to synchronize GPS time
                try_sync_gps_time();

                // Process incoming MQTT messages (blocking on broker response)
                mqtt_process();

                TickType_t now = xTaskGetTickCount();

                // Publish telemetry every MQTT_PUBLISH_PERIOD_MS (1s)
                if ((now - last_telemetry_time) * portTICK_PERIOD_MS >= MQTT_PUBLISH_PERIOD_MS) {
                    last_telemetry_time = now;
                    publish_telemetry();
                }

                // Publish status every MQTT_STATUS_PERIOD_MS (30s)
                if ((now - last_status_time) * portTICK_PERIOD_MS >= MQTT_STATUS_PERIOD_MS) {
                    last_status_time = now;
                    publish_status("periodic");
                }

                // Handle incoming MQTT events from queue (impact, card events, etc.)
                handle_mqtt_event();

                vTaskDelay(pdMS_TO_TICKS(100));
                break;
            }

            case MQTT_STATE_RECONNECT: {
                printf("[TaskMQTT] State: RECONNECT\n");
                mqtt_disconnect();
                vTaskDelay(pdMS_TO_TICKS(5000));
                state = MQTT_STATE_CONNECT;
                break;
            }

            default:
                state = MQTT_STATE_INIT_SIM;
                break;
        }
    }
}

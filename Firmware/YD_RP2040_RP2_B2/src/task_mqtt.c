#include "task_mqtt.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#include "my_mqtt.h"
#include "sim7680.h"
#include "algorithm_bmi160.h"
#include "neo6m.h"
#include "task_bmi160.h"
#include "task_gps.h"
#include "task_buzzer.h"

#define MQTT_PUBLISH_PERIOD_MS 1000

typedef enum {
    MQTT_STATE_INIT_SIM = 0,
    MQTT_STATE_CONNECT,
    MQTT_STATE_SUBSCRIBE,
    MQTT_STATE_RUNNING,
    MQTT_STATE_RECONNECT
} MQTT_State_t;

void mqtt_message_received(const char *topic, const char *payload) {
    printf("[main] MQTT_RX [%s]: %s\n", topic, payload);
    Buzzer_SendCmd(BUZZER_CMD_BEEP_SHORT);
}

void TaskMQTT(void *pvParameters) {
    (void)pvParameters;
    MQTT_State_t state = MQTT_STATE_INIT_SIM;
    TickType_t last_pub_time = 0;

    for (;;) {
        switch (state) {
            case MQTT_STATE_INIT_SIM: {
                printf("[TaskMQTT] Trang thai: INIT_SIM\n");
                sim7680_init();
                printf("[TaskMQTT] Cho SIM ready...\n");
                while (!sim7680_wait_ready(10000)) {
                    printf("[TaskMQTT] SIM chua ready, thu lai...\n");
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }

                bool sim_ready = false;
                while (!sim7680_check_sim(&sim_ready) || !sim_ready) {
                    printf("[TaskMQTT] Cho nhan the SIM...\n");
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }
                printf("[TaskMQTT] SIM san sang.\n");
                state = MQTT_STATE_CONNECT;
                break;
            }

            case MQTT_STATE_CONNECT: {
                printf("[TaskMQTT] Trang thai: CONNECT\n");
                if (!mqtt_init()) {
                    printf("[TaskMQTT] Lỗi mqtt_init(). Thu lai sau 5s...\n");
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    break;
                }
                if (mqtt_connect()) {
                    state = MQTT_STATE_SUBSCRIBE;
                } else {
                    printf("[TaskMQTT] Lỗi mqtt_connect(). Thu lai sau 5s...\n");
                    vTaskDelay(pdMS_TO_TICKS(5000));
                }
                break;
            }

            case MQTT_STATE_SUBSCRIBE: {
                printf("[TaskMQTT] Trang thai: SUBSCRIBE\n");
                if (mqtt_subscribe(MQTT_TOPIC_COMMAND)) {
                    printf("[TaskMQTT] Subscribe thanh cong: %s\n", MQTT_TOPIC_COMMAND);
                    state = MQTT_STATE_RUNNING;
                    last_pub_time = xTaskGetTickCount();
                } else {
                    printf("[TaskMQTT] Loi Subscribe, thu reconnect...\n");
                    state = MQTT_STATE_RECONNECT;
                }
                break;
            }

            case MQTT_STATE_RUNNING: {
                mqtt_process();
                TickType_t now = xTaskGetTickCount();
                if ((now - last_pub_time) * portTICK_PERIOD_MS >= MQTT_PUBLISH_PERIOD_MS) {
                    last_pub_time = now;
                    BMI160_Physical_t bmi = {0};
                    neo6m_data_t gps = {0};
                    bool has_bmi = BMI160_GetLastData(&bmi);
                    bool has_gps = GPS_GetLastData(&gps);

                    char payload[256];
                    snprintf(payload, sizeof(payload),
                             "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f,\"spd\":%.1f,\"accX\":%.2f,\"accY\":%.2f,\"accZ\":%.2f}",
                             has_gps ? gps.latitude : 0.0f,
                             has_gps ? gps.longitude : 0.0f,
                             has_gps ? gps.altitude_m : 0.0f,
                             has_gps ? gps.speed_kmh : 0.0f,
                             has_bmi ? bmi.acc_x_g : 0.0f,
                             has_bmi ? bmi.acc_y_g : 0.0f,
                             has_bmi ? bmi.acc_z_g : 0.0f);

                    printf("[TaskMQTT] Publishing: %s\n", payload);
                    if (!mqtt_publish(MQTT_TOPIC_TELEMETRY, payload, false)) {
                        printf("[TaskMQTT] Publish that bai! Chuyen sang RECONNECT.\n");
                        state = MQTT_STATE_RECONNECT;
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
            }

            case MQTT_STATE_RECONNECT: {
                printf("[TaskMQTT] Trang thai: RECONNECT\n");
                mqtt_disconnect();
                vTaskDelay(pdMS_TO_TICKS(5000));
                state = MQTT_STATE_CONNECT;
                break;
            }
        }
    }
}

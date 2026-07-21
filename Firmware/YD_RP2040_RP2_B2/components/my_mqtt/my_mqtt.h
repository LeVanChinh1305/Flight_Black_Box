#ifndef MY_MQTT_H
#define MY_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Cấu hình MQTT
#define MQTT_CLIENT_ID      "FlightBlackBox_RP2040"
#define MQTT_BROKER         "broker.hivemq.com"
#define MQTT_PORT           1883
#define MQTT_KEEPALIVE      60
#define MQTT_QOS            0          
#define MQTT_RETAIN         0

// Topic mặc định
#define MQTT_TOPIC_TELEMETRY   "flight/blackbox/telemetry"
#define MQTT_TOPIC_COMMAND     "flight/blackbox/command"     

bool mqtt_init(void);                                                        // Khởi tạo MQTT client
bool mqtt_connect(void);                                                     // Kết nối đến broker MQTT
bool mqtt_disconnect(void);                                                  // Ngắt kết nối khỏi broker MQTT
bool mqtt_publish(const char* topic, const char* payload, bool retain);      // Gửi dữ liệu lên broker MQTT
bool mqtt_subscribe(const char* topic);                                      // Đăng ký nhận tin nhắn từ topic
bool mqtt_unsubscribe(const char* topic);                                    // Hủy đăng ký nhận tin nhắn từ topic
bool mqtt_process(void);                                                     // Xử lý các sự kiện MQTT

extern void mqtt_message_received(const char* topic, const char* payload);

#ifdef __cplusplus
}
#endif

#endif // MY_MQTT_H
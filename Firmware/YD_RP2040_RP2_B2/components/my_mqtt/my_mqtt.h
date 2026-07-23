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

// Topic định nghĩa theo spec (không dùng device ID, chỉ chạy 1 thiết bị trên broker)
#define MQTT_TOPIC_TELEMETRY   "flightbox/publish/telemetry"
#define MQTT_TOPIC_STATUS      "flightbox/publish/status"
#define MQTT_TOPIC_EVENT       "flightbox/publish/event"
#define MQTT_TOPIC_CARD        "flightbox/publish/card"
#define MQTT_TOPIC_ACK         "flightbox/publish/ack"
#define MQTT_TOPIC_COMMAND     "flightbox/subscribe/command"

// Chu kỳ publish (tính bằng ms)
#define MQTT_PUBLISH_PERIOD_MS       1000  // telemetry: 1s
#define MQTT_STATUS_PERIOD_MS        30000 // status: 30s
#define MQTT_CARD_PERIODIC_MS        60000 // card: 60s     
#define MQTT_QOS                     1     // Quality of Service cho publish
#define MQTT_RETAIN                  0     // Retain flag (mặc định false, riêng status và full card là true)

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
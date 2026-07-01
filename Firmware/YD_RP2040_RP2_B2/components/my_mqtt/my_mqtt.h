#ifndef MY_MQTT_H
#define MY_MQTT_H

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

bool mqtt_init(void);
bool mqtt_connect(void);
bool mqtt_disconnect(void);
bool mqtt_publish(const char* topic, const char* payload, bool retain);
bool mqtt_subscribe(const char* topic);
bool mqtt_unsubscribe(const char* topic);
bool mqtt_process(void);

extern void mqtt_message_received(const char* topic, const char* payload);

#endif // MY_MQTT_H
#include "my_mqtt.h"
#include "FreeRTOS.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "sim7680.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

static bool mqtt_started = false;
static bool mqtt_connected = false;

void __attribute__((weak)) mqtt_message_received(const char *topic,
                                                 const char *payload) {
  printf("[MQTT] Received [%s]: %s\n", topic, payload);
}

static bool send_at_cmd(const char *cmd, uint32_t timeout_ms) {
  char resp[128];
  bool ok = sim7680_send_cmd(cmd, resp, sizeof(resp), timeout_ms);
  if (!ok) {
    printf("[MQTT] CMD FAIL: %s -> %s\n", cmd, resp);   // Chỉ in khi lỗi
  }
  return ok;
}
 
// Gửi lệnh mở prompt '>' rồi gửi dữ liệu thô (topic/payload) kết thúc bằng
// Ctrl+Z. Không còn tự đọc UART trực tiếp nữa - dùng chung hàng đợi phản hồi
// với mọi lệnh AT khác (sim7680_wait_prompt / sim7680_send_cmd), vì vậy
// không có nguy cơ giẫm chân / xóa mất dữ liệu URC đang đến song song.
static bool send_with_prompt(const char *cmd, const char *data,
                             uint32_t timeout_ms) {
  uart_puts(SIM7680_UART, cmd);
  uart_puts(SIM7680_UART, "\r\n");

  if (!sim7680_wait_prompt(timeout_ms)) {
    printf("[MQTT] Timeout: Khong nhan duoc '>' cho lenh %s\n", cmd);
    return false;
  }

  // Gửi data + Ctrl+Z
  uart_puts(SIM7680_UART, data);
  uart_putc(SIM7680_UART, 0x1A);

  // Chờ "OK" cho việc gửi data này - không gửi thêm lệnh AT nào,
  // chỉ đọc tiếp từ hàng đợi phản hồi.
  char resp[128] = {0};
  return sim7680_read_ok(resp, sizeof(resp), timeout_ms);
}

static bool setup_pdp_context(void) {
  printf("[NET] Cấu hình PDP...\n");
  if (!send_at_cmd("AT+CGDCONT=1,\"IP\",\"viettel\"", 3000)) {
    return false;
  }
  if (!send_at_cmd("AT+CGACT=1,1", 5000)) {
    return false;
  }
  return send_at_cmd("AT+CGPADDR=1", 3000);
}

bool mqtt_init(void) {
  if (mqtt_started)
    return true;

  printf("[MQTT] Starting MQTT service...\n");

  printf("[NET] Khởi động sạch (Clean Boot) stack mạng...\n");
  send_at_cmd("AT+CFUN=0", 3000);
  vTaskDelay(pdMS_TO_TICKS(2000));
  send_at_cmd("AT+CFUN=1", 5000);
  vTaskDelay(pdMS_TO_TICKS(3000));

  if (!sim7680_wait_network_ready(20000)) {
    printf("[MQTT] Network not ready after CFUN reset\n");
    return false;
  }

  if (!setup_pdp_context()) {
    printf("[MQTT] PDP context setup failed\n");
    return false;
  }

  char test_resp[128];
  bool supports_mqtt =
      sim7680_send_cmd("AT+CMQTTSTART=?", test_resp, sizeof(test_resp), 2000);
  if (!supports_mqtt)
    return false;

  send_at_cmd("AT+CMQTTDISC=0,120", 2000);
  send_at_cmd("AT+CMQTTREL=0", 2000);
  send_at_cmd("AT+CMQTTSTOP", 2000);

  if (send_at_cmd("AT+CMQTTSTART", 5000)) {
    mqtt_started = true;
    return true;
  }
  return false;
}

bool mqtt_connect(void) {
  if (!mqtt_started && !mqtt_init())
    return false;

  char cmd[120];
  snprintf(cmd, sizeof(cmd), "AT+CMQTTACCQ=0,\"%s\"", MQTT_CLIENT_ID);
  if (!send_at_cmd(cmd, 3000))
    return false;

  snprintf(cmd, sizeof(cmd), "AT+CMQTTCONNECT=0,\"tcp://%s:%d\",%d,1",
           MQTT_BROKER, MQTT_PORT, MQTT_KEEPALIVE);

  if (send_at_cmd(cmd, 15000)) {
    mqtt_connected = true;
    printf("[MQTT] Connected successfully to HiveMQ!\n");
    return true;
  }
  return false;
}

bool mqtt_publish(const char *topic, const char *payload, bool retain) {
  if (!mqtt_connected)
    return false;

  int topic_len = strlen(topic);
  int payload_len = strlen(payload);

  char cmd_topic[64];
  snprintf(cmd_topic, sizeof(cmd_topic), "AT+CMQTTTOPIC=0,%d", topic_len);
  if (!send_with_prompt(cmd_topic, topic, 4000))
    return false;

  char cmd_payload[64];
  snprintf(cmd_payload, sizeof(cmd_payload), "AT+CMQTTPAYLOAD=0,%d",
           payload_len);
  if (!send_with_prompt(cmd_payload, payload, 4000))
    return false;

  char pub_cmd[64];
  snprintf(pub_cmd, sizeof(pub_cmd), "AT+CMQTTPUB=0,%d,%d,%d", MQTT_QOS,
           MQTT_KEEPALIVE, retain ? 1 : 0);
  return send_at_cmd(pub_cmd, 10000);
}

bool mqtt_subscribe(const char *topic) {
  if (!mqtt_connected)
    return false;
  printf("[MQTT] Subscribing to %s ...\n", topic);

  int topic_len = strlen(topic);
  char cmd_sub[64];
  snprintf(cmd_sub, sizeof(cmd_sub), "AT+CMQTTSUBTOPIC=0,%d,%d", topic_len,
           MQTT_QOS);
  if (!send_with_prompt(cmd_sub, topic, 4000))
    return false;

  return send_at_cmd("AT+CMQTTSUB=0", 5000);
}

// URC "+CMRECV:" giờ được sim7680_rx_task (trong sim7680.cpp) chặn và gọi
// mqtt_message_received() ngay tại chỗ, độc lập hoàn toàn với việc publish
// hay bất kỳ lệnh AT nào khác đang chạy. Vì vậy hàm này không còn cần đọc
// UART nữa - giữ lại chỉ để không phải sửa main.cpp (TaskMQTT vẫn gọi nó).
bool mqtt_process(void) { return true; }

bool mqtt_disconnect(void) { return true; }
bool mqtt_unsubscribe(const char *topic) {
  (void)topic;
  return true;
} // TODO: chua trien khai that
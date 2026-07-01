#ifndef SIM7680_H
#define SIM7680_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* ================== Cấu hình phần cứng ==================
 * Module SIM  TX  ---> RP2040 GPIO5  (RX của RP2040)
 * Module SIM  RX  <--- RP2040 GPIO4  (TX của RP2040)
 * => Trên RP2040 dùng UART1 (mapping mặc định: TX=GPIO4, RX=GPIO5)
 * ========================================================== */
#define SIM7680_UART        uart1
#define SIM7680_UART_TX_PIN 4      // RP2040 TX -> SIM RX
#define SIM7680_UART_RX_PIN 5      // RP2040 RX <- SIM TX
#define SIM7680_BAUDRATE    115200

// Khởi tạo UART giao tiếp với module SIM (gọi 1 lần lúc đầu chương trình)
void sim7680_init(void);

// Gửi 1 lệnh AT, đọc phản hồi thô vào buffer.
// Trả về true nếu phản hồi có chứa "OK", false nếu ERROR/timeout.
bool sim7680_send_cmd(const char *cmd, char *response, size_t resp_size, uint32_t timeout_ms);

// Đợi module boot xong bằng cách gửi "AT" lặp lại cho tới khi có phản hồi OK
bool sim7680_wait_ready(uint32_t timeout_ms);

// Kiểm tra module còn phản hồi AT không ("AT" -> "OK")
bool sim7680_test(void);

// Lấy IMEI của module (AT+CGSN)
bool sim7680_get_imei(char *imei, size_t size);

// Lấy tên model module (ATI)
bool sim7680_get_model(char *model, size_t size);

// Lấy chất lượng tín hiệu AT+CSQ -> rssi:0-31,99  ber:0-7,99
bool sim7680_get_signal_quality(int *rssi, int *ber);

// Lấy trạng thái đăng ký mạng AT+CREG? -> status: 0..5
bool sim7680_get_network_status(int *status);

// Kiểm tra SIM đã được nhận diện chưa (AT+CPIN?). ready=true nếu SIM sẵn sàng (READY).
bool sim7680_check_sim(bool *ready);

// Kiểm tra trạng thái chức năng vô tuyến (AT+CFUN?). 0=tat song,1=bat day du,4=che do bay
bool sim7680_get_radio_function(int *cfun);

#endif // SIM7680_H
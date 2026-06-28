#ifndef SIM7080G_H
#define SIM7080G_H

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <stdbool.h>

// ── Phần cứng ──────────────────────────────────────────────
#define SIM_UART_PORT   uart1
#define SIM_BAUD_RATE   115200
#define PIN_SIM_TX      4
#define PIN_SIM_RX      5
#define PIN_SIM_PWR     2

// ── Tham số thời gian ──────────────────────────────────────
#define SIM_PWRKEY_PULSE_MS     1500    // Độ rộng xung PWRKEY (ms)
#define SIM_BOOT_WAIT_MS        5000    // Chờ module boot xong (ms)
#define SIM_DEFAULT_TIMEOUT_MS  3000    // Timeout lệnh thông thường (ms)
#define SIM_LONG_TIMEOUT_MS     15000   // Timeout lệnh chậm - đăng ký mạng (ms)
#define SIM_RX_BUF_SIZE         512     // Kích thước buffer nhận UART

// ── Mã kết quả ─────────────────────────────────────────────
typedef enum {
    SIM_OK            =  0,
    SIM_ERR_TIMEOUT   = -1,   // Không nhận được phản hồi đúng hạn
    SIM_ERR_NO_SIM    = -2,   // Không có SIM hoặc PIN bị khóa
    SIM_ERR_NO_NET    = -3,   // Chưa đăng ký được mạng
    SIM_ERR_GENERIC   = -4,   // Module trả về ERROR
} SimResult;

// ── API công khai ───────────────────────────────────────────

// Khởi tạo UART, kích nguồn PWRKEY, đồng bộ baud rate
SimResult sim_init(void);

// Gửi lệnh AT, chờ đến khi nhận "OK" hoặc "ERROR" hoặc timeout
SimResult sim_send_at(const char* cmd,
                      char*       resp_buf,
                      uint16_t    buf_size,
                      uint32_t    timeout_ms);

// Kiểm tra SIM card đã sẵn sàng chưa (AT+CPIN?)
SimResult sim_check_sim(void);

// Chờ đăng ký mạng (CEREG stat=1 hoặc 5)
SimResult sim_wait_network(uint32_t timeout_ms);

// Cấu hình APN và kích hoạt PDP context để có data
SimResult sim_connect_gprs(const char* apn);

// In toàn bộ thông tin hệ thống, cấu hình vô tuyến và mạng
void sim_get_all_diagnostics(void);

// ── Tiện ích nội bộ (dùng trong sim7080g.cpp) ────────────────
void sim_flush_rx(void);
bool sim_read_until(const char* keyword, char* buf,
                    uint16_t buf_size, uint32_t timeout_ms);
bool sim_contains(const char* buf, const char* keyword);

#endif // SIM7080G_H
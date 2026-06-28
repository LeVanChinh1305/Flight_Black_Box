#include "sim7080g.h"
#include <stdio.h>
#include <string.h>

// ============================================================
//  TIỆN ÍCH NỘI BỘ
// ============================================================

void sim_flush_rx(void) {
    while (uart_is_readable(SIM_UART_PORT))
        uart_getc(SIM_UART_PORT);
    sleep_ms(20);
    while (uart_is_readable(SIM_UART_PORT))
        uart_getc(SIM_UART_PORT);
    sleep_ms(5);
}

bool sim_contains(const char* buf, const char* keyword) {
    return strstr(buf, keyword) != NULL;
}

bool sim_read_until(const char* keyword, char* buf,
                    uint16_t buf_size, uint32_t timeout_ms) {
    uint16_t idx = 0;
    uint32_t elapsed = 0;

    memset(buf, 0, buf_size);

    while (elapsed < timeout_ms) {
        if (uart_is_readable(SIM_UART_PORT)) {
            char c = uart_getc(SIM_UART_PORT);

            if ((c >= 0x20 && c <= 0x7E) || c == '\r' || c == '\n') {
                if (idx < buf_size - 1) {
                    buf[idx++] = c;
                    buf[idx]   = '\0';
                }
            }
            if (sim_contains(buf, keyword)) return true;

        } else {
            sleep_ms(1);
            elapsed += 1;
        }
    }
    return false;
}

// ============================================================
//  ĐỒNG BỘ BAUD RATE
// ============================================================

static bool sim_sync_baud(uint8_t max_tries) {
    char buf[64];
    uint8_t ok_count = 0;

    for (uint8_t i = 0; i < max_tries; i++) {
        sim_flush_rx();
        uart_puts(SIM_UART_PORT, "AT\r\n");

        bool got_ok = sim_read_until("OK", buf, sizeof(buf), 500);

        if (got_ok) {
            ok_count++;
            printf("[SIM] Sync %d/2: OK\n", ok_count);
            if (ok_count >= 2) return true;
        } else {
            ok_count = 0;
            printf("[SIM] Sync attempt %d: no OK (got: %s)\n", i + 1, buf);
        }
        sleep_ms(200);
    }
    return false;
}

// ============================================================
//  KHỞI TẠO
// ============================================================

SimResult sim_init(void) {
    uart_init(SIM_UART_PORT, SIM_BAUD_RATE);
    gpio_set_function(PIN_SIM_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_SIM_RX, GPIO_FUNC_UART);

    gpio_init(PIN_SIM_PWR);
    gpio_set_dir(PIN_SIM_PWR, GPIO_OUT);
    gpio_put(PIN_SIM_PWR, 1);
    sleep_ms(100);

    printf("[SIM] Dang kich nguon SIM7080G...\n");
    gpio_put(PIN_SIM_PWR, 0);               // Kéo PWRKEY xuống LOW
    sleep_ms(SIM_PWRKEY_PULSE_MS);          // Giữ 1500ms
    gpio_put(PIN_SIM_PWR, 1);              // Thả về HIGH và GIỮ NGUYÊN HIGH
    // KHÔNG thả float chân PWR — giữ OUTPUT HIGH để tránh trigger reboot ngẫu nhiên

    printf("[SIM] Cho module boot (%d ms)...\n", SIM_BOOT_WAIT_MS);
    sleep_ms(SIM_BOOT_WAIT_MS);

    printf("[SIM] Dang dong bo baud rate...\n");
    if (!sim_sync_baud(10)) {
        printf("[SIM] LỖI: Không đồng bộ được baud rate!\n");
        return SIM_ERR_TIMEOUT;
    }

    char buf[64];
    sim_send_at("AT+IPR=115200\r\n", buf, sizeof(buf), SIM_DEFAULT_TIMEOUT_MS);
    printf("[SIM] Baud rate da khoa: 115200\n");

    sim_send_at("ATE0\r\n", buf, sizeof(buf), SIM_DEFAULT_TIMEOUT_MS);
    printf("[SIM] Echo da tat (ATE0)\n");

    printf("[SIM] Khoi tao hoan tat!\n");
    return SIM_OK;
}

// ============================================================
//  GỬI LỆNH AT
// ============================================================

SimResult sim_send_at(const char* cmd, char* resp_buf,
                      uint16_t buf_size, uint32_t timeout_ms) {
    char local_buf[SIM_RX_BUF_SIZE];
    char* buf  = (resp_buf && buf_size > 0) ? resp_buf : local_buf;
    uint16_t sz = (resp_buf && buf_size > 0) ? buf_size : sizeof(local_buf);

    sim_flush_rx();

    printf("[SIM TX] %s", cmd);
    uart_puts(SIM_UART_PORT, cmd);

    bool got_ok  = sim_read_until("OK",    buf, sz, timeout_ms);
    bool got_err = !got_ok && sim_contains(buf, "ERROR");

    printf("[SIM RX]\n%s\n---------\n", buf);

    if (got_ok)  return SIM_OK;
    if (got_err) return SIM_ERR_GENERIC;
    return SIM_ERR_TIMEOUT;
}

// ============================================================
//  KIỂM TRA SIM CARD
// ============================================================

SimResult sim_check_sim(void) {
    char buf[128];
    SimResult r = sim_send_at("AT+CPIN?\r\n", buf, sizeof(buf), SIM_DEFAULT_TIMEOUT_MS);
    if (r != SIM_OK) return r;
    if (!sim_contains(buf, "READY")) {
        printf("[SIM] SIM khong san sang: %s\n", buf);
        return SIM_ERR_NO_SIM;
    }
    printf("[SIM] SIM OK\n");
    return SIM_OK;
}

// ============================================================
//  CHỜ ĐĂNG KÝ MẠNG
// ============================================================

SimResult sim_wait_network(uint32_t timeout_ms) {
    char buf[128];
    uint32_t elapsed = 0;
    const uint32_t poll_interval = 2000;

    printf("[SIM] Dang cho dang ky mang...\n");
    while (elapsed < timeout_ms) {
        sim_send_at("AT+CEREG?\r\n", buf, sizeof(buf), SIM_DEFAULT_TIMEOUT_MS);

        if (sim_contains(buf, "+CEREG: 0,1") ||
            sim_contains(buf, "+CEREG: 0,5") ||
            sim_contains(buf, "+CEREG: 2,1") ||
            sim_contains(buf, "+CEREG: 2,5")) {
            printf("[SIM] Da dang ky mang!\n");
            return SIM_OK;
        }
        printf("[SIM] Chua co mang (%lu ms)...\n", elapsed);
        sleep_ms(poll_interval);
        elapsed += poll_interval;
    }
    printf("[SIM] Timeout: khong dang ky duoc mang!\n");
    return SIM_ERR_NO_NET;
}

// ============================================================
//  KẾT NỐI GPRS / NB-IoT
// ============================================================

SimResult sim_connect_gprs(const char* apn) {
    char buf[256];
    char cmd[128];

    snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", apn);
    if (sim_send_at(cmd, buf, sizeof(buf), SIM_DEFAULT_TIMEOUT_MS) != SIM_OK) {
        printf("[SIM] Loi cau hinh APN\n");
        return SIM_ERR_GENERIC;
    }

    if (sim_send_at("AT+CGATT=1\r\n", buf, sizeof(buf), SIM_LONG_TIMEOUT_MS) != SIM_OK) {
        printf("[SIM] Loi GPRS attach\n");
        return SIM_ERR_GENERIC;
    }

    if (sim_send_at("AT+CGACT=1,1\r\n", buf, sizeof(buf), SIM_LONG_TIMEOUT_MS) != SIM_OK) {
        printf("[SIM] Loi kich hoat PDP context\n");
        return SIM_ERR_GENERIC;
    }

    sim_send_at("AT+CGPADDR=1\r\n", buf, sizeof(buf), SIM_DEFAULT_TIMEOUT_MS);
    printf("[SIM] IP: %s\n", buf);

    return SIM_OK;
}

// ============================================================
//  QUÉT TOÀN BỘ CHỈ SỐ LỆNH YÊU CẦU
// ============================================================

void sim_get_all_diagnostics(void) {
    // Tách riêng mảng đệm lớn cho lệnh quét nhà mạng diện rộng để tránh tràn RAM
    char high_capacity_buf[1024]; 
    char normal_buf[256];

    printf("\n=================================================\n");
    printf("   TIẾN HÀNH THU THẬP TẤT CẢ TÍN HIỆU DIAGNOSTICS\n");
    printf("=================================================\n");

    // 1. BASIC INFORMATION
    sim_send_at("ATI\r\n", normal_buf, sizeof(normal_buf), SIM_DEFAULT_TIMEOUT_MS);
    sim_send_at("AT+CGMR\r\n", normal_buf, sizeof(normal_buf), SIM_DEFAULT_TIMEOUT_MS);
    sim_send_at("AT+CIMI\r\n", normal_buf, sizeof(normal_buf), SIM_DEFAULT_TIMEOUT_MS);
    sim_send_at("AT+ICCID\r\n", normal_buf, sizeof(normal_buf), SIM_DEFAULT_TIMEOUT_MS);

    // 2. RADIO CONFIGURATIONS
    sim_send_at("AT+CNMP?\r\n", normal_buf, sizeof(normal_buf), SIM_DEFAULT_TIMEOUT_MS);
    sim_send_at("AT+CMNB?\r\n", normal_buf, sizeof(normal_buf), SIM_DEFAULT_TIMEOUT_MS);
    sim_send_at("AT+CBANDCFG?\r\n", normal_buf, sizeof(normal_buf), SIM_DEFAULT_TIMEOUT_MS);

    // 3. NETWORK DIAGNOSTICS
    sim_send_at("AT+COPS?\r\n", normal_buf, sizeof(normal_buf), SIM_DEFAULT_TIMEOUT_MS);
    
    // AT+COPS=? bị loại bỏ — lệnh này reset trạng thái đăng ký mạng về 0
    // làm module ngừng tìm kiếm CAT-M sau khi chạy xong diagnostics

    // 4. SIGNAL STRENGTH
    sim_send_at("AT+CSQ\r\n", normal_buf, sizeof(normal_buf), SIM_DEFAULT_TIMEOUT_MS);
    sim_send_at("AT+CESQ\r\n", normal_buf, sizeof(normal_buf), SIM_DEFAULT_TIMEOUT_MS);

    // 5. REGISTRATION & DATA PACKET
    sim_send_at("AT+CEREG?\r\n", normal_buf, sizeof(normal_buf), SIM_DEFAULT_TIMEOUT_MS);
    sim_send_at("AT+CGATT?\r\n", normal_buf, sizeof(normal_buf), SIM_DEFAULT_TIMEOUT_MS);
    
    printf("=================================================\n\n");
}
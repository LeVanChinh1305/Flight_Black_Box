#include "sim7680.h"

#include <string.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"

// ---------- Hàm nội bộ ----------

// Xả sạch dữ liệu cũ còn nằm trong bộ đệm RX trước khi gửi lệnh mới
static void sim7680_flush_rx(void)
{
    while (uart_is_readable(SIM7680_UART)) {
        (void)uart_getc(SIM7680_UART);
    }
}

// Đọc phản hồi từ module cho tới khi gặp "OK"/"ERROR" hoặc hết timeout
static bool sim7680_read_response(char *response, size_t resp_size, uint32_t timeout_ms)
{
    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
    size_t idx = 0;

    if (response && resp_size > 0) {
        response[0] = '\0';
    }

    while (absolute_time_diff_us(get_absolute_time(), deadline) > 0) {
        if (uart_is_readable(SIM7680_UART)) {
            char c = uart_getc(SIM7680_UART);

            if (response && idx < resp_size - 1) {
                response[idx++] = c;
                response[idx] = '\0';
            }

            if (response && strstr(response, "OK\r\n") != NULL) {
                return true;
            }
            if (response && strstr(response, "ERROR") != NULL) {
                return false;
            }
        }
    }

    // Hết thời gian chờ nhưng có thể đã nhận được OK ở giữa chuỗi
    if (response && strstr(response, "OK") != NULL) {
        return true;
    }
    return false;
}

// ---------- Hàm public ----------

void sim7680_init(void)
{
    uart_init(SIM7680_UART, SIM7680_BAUDRATE);

    gpio_set_function(SIM7680_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(SIM7680_UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_hw_flow(SIM7680_UART, false, false);
    uart_set_format(SIM7680_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(SIM7680_UART, true);
}

bool sim7680_send_cmd(const char *cmd, char *response, size_t resp_size, uint32_t timeout_ms)
{
    sim7680_flush_rx();

    uart_puts(SIM7680_UART, cmd);
    uart_puts(SIM7680_UART, "\r\n");

    return sim7680_read_response(response, resp_size, timeout_ms);
}

bool sim7680_wait_ready(uint32_t timeout_ms)
{
    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
    char resp[64];

    while (absolute_time_diff_us(get_absolute_time(), deadline) > 0) {
        if (sim7680_send_cmd("AT", resp, sizeof(resp), 1000)) {
            return true;
        }
        sleep_ms(500);
    }
    return false;
}

bool sim7680_test(void)
{
    char resp[64];
    return sim7680_send_cmd("AT", resp, sizeof(resp), 1000);
}

bool sim7680_get_imei(char *imei, size_t size)
{
    char resp[64];
    if (!sim7680_send_cmd("AT+CGSN", resp, sizeof(resp), 1000)) {
        return false;
    }

    // Phản hồi dạng: "<số IMEI 15 chữ số>\r\n\r\nOK\r\n"
    for (size_t i = 0; i < strlen(resp); i++) {
        if (resp[i] >= '0' && resp[i] <= '9') {
            size_t j = 0;
            while (resp[i] >= '0' && resp[i] <= '9' && j < size - 1) {
                imei[j++] = resp[i++];
            }
            imei[j] = '\0';
            return j > 0;
        }
    }
    return false;
}

bool sim7680_get_model(char *model, size_t size)
{
    char resp[128];
    if (!sim7680_send_cmd("ATI", resp, sizeof(resp), 1000)) {
        return false;
    }
    strncpy(model, resp, size - 1);
    model[size - 1] = '\0';
    return true;
}

bool sim7680_get_signal_quality(int *rssi, int *ber)
{
    char resp[64];
    if (!sim7680_send_cmd("AT+CSQ", resp, sizeof(resp), 1000)) {
        return false;
    }

    char *p = strstr(resp, "+CSQ:");
    if (!p) {
        return false;
    }
    return sscanf(p, "+CSQ: %d,%d", rssi, ber) == 2;
}

bool sim7680_get_network_status(int *status)
{
    char resp[64];
    if (!sim7680_send_cmd("AT+CREG?", resp, sizeof(resp), 1000)) {
        return false;
    }

    char *p = strstr(resp, "+CREG:");
    if (!p) {
        return false;
    }
    int n;
    return sscanf(p, "+CREG: %d,%d", &n, status) == 2;
}

bool sim7680_check_sim(bool *ready)
{
    char resp[64];
    if (!sim7680_send_cmd("AT+CPIN?", resp, sizeof(resp), 2000)) {
        *ready = false;
        return false;
    }
    *ready = (strstr(resp, "+CPIN: READY") != NULL);
    return true;
}

bool sim7680_get_radio_function(int *cfun)
{
    char resp[64];
    if (!sim7680_send_cmd("AT+CFUN?", resp, sizeof(resp), 1000)) {
        return false;
    }
    char *p = strstr(resp, "+CFUN:");
    if (!p) {
        return false;
    }
    return sscanf(p, "+CFUN: %d", cfun) == 1;
}



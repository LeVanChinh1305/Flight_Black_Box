#include "sim7680.h"
#include "my_mqtt.h"   // mqtt_message_received(), MQTT_TOPIC_COMMAND

#include <string.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// =====================================================================
// LỚP 1: RING BUFFER CẤP BYTE (ISR-safe, single-producer/single-consumer)
// =====================================================================
// ISR ghi vào head, sim7680_rx_task đọc từ tail. Không cần disable-IRQ để
// bảo vệ vì head/tail là kiểu 16-bit, đọc/ghi atomic trên Cortex-M0+.
#define RB_SIZE 512  // phải là lũy thừa của 2

typedef struct {
    volatile uint8_t  buf[RB_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} ring_buffer_t;

static ring_buffer_t rb = {0};
// KHÔNG dùng semaphore nữa để tiết kiệm heap (board này chỉ có 48KB heap cho
// FreeRTOS, đã gần cạn do các task khác + FatFs). RX task poll ring buffer
// mỗi 2ms - với buffer 512 byte @115200bps thì dư sức, không mất dữ liệu.
// Độ trễ 2ms là không đáng kể so với bug gốc (mất hẳn dữ liệu do bị flush).

static inline void rb_push_isr(uint8_t c)
{
    uint16_t next = (uint16_t)((rb.head + 1) & (RB_SIZE - 1));
    if (next != rb.tail) {   // còn chỗ trống mới ghi, tránh ghi đè dữ liệu chưa đọc
        rb.buf[rb.head] = c;
        rb.head = next;
    }
    // Nếu đầy (gần như không xảy ra ở 115200bps với RX task đọc liên tục)
    // thì đành bỏ byte mới nhất, còn hơn crash hay khóa ISR.
}

static inline bool rb_pop(uint8_t *c)
{
    if (rb.tail == rb.head) return false; // rỗng
    *c = rb.buf[rb.tail];
    rb.tail = (uint16_t)((rb.tail + 1) & (RB_SIZE - 1));
    return true;
}

// =====================================================================
// LỚP 2: UART INTERRUPT HANDLER
// =====================================================================
// Chỉ làm đúng 1 việc: rút byte ra khỏi FIFO phần cứng của UART càng
// nhanh càng tốt và nhét vào ring buffer. Không printf, không delay,
// không xử lý logic gì ở đây.
static void sim7680_uart_isr(void)
{
    while (uart_is_readable(SIM7680_UART)) {
        uint8_t c = (uint8_t)uart_getc(SIM7680_UART);
        rb_push_isr(c);
    }
    // Không dùng semaphore - RX task tự poll ring buffer định kỳ (xem sim7680_rx_task).
}

// =====================================================================
// LỚP 3: RX TASK - tách URC (+CMRECV:) ra khỏi response của lệnh AT
// =====================================================================
// Chỉ 96 byte/dòng thay vì 256, và chỉ 4 dòng trong hàng đợi thay vì 8
// -> queue chỉ tốn ~400 byte heap thay vì 2048 byte như bản trước.
// Đủ dùng vì các dòng phản hồi AT thường (OK/ERROR/+CSQ:.../+CPIN:...)
// đều rất ngắn; riêng "+CMRECV:" (có thể dài do chứa payload) KHÔNG đi
// qua queue này - nó được xử lý trực tiếp ngay trong RX task bên dưới.
typedef struct { char text[96]; } sim_line_t;

static QueueHandle_t resp_queue = NULL; // các dòng KHÔNG phải URC, dành cho sim7680_read_response()

static void sim7680_rx_task(void *arg)
{
    (void)arg;
    static char line[256]; // buffer dựng dòng nằm trên stack task, không tốn heap
    static size_t len = 0;

    static bool receiving_mqtt = false;
    static int mqtt_rx_state = 0; // 0: chờ, 1: chuẩn bị đọc topic, 2: chuẩn bị đọc payload
    static char mqtt_payload[256];

    for (;;) {
        // Poll ring buffer mỗi 2ms
        vTaskDelay(pdMS_TO_TICKS(2));

        uint8_t c;
        while (rb_pop(&c)) {

            // Module gửi prompt '>' KHÔNG kèm '\n' theo sau ngay lập tức
            if (c == '>' && len == 0 && !receiving_mqtt) {
                sim_line_t l;
                strcpy(l.text, ">");
                xQueueSend(resp_queue, &l, 0);
                continue;
            }

            if (len < sizeof(line) - 1) {
                line[len++] = (char)c;
                line[len] = '\0';
            }

            if (c == '\n') {
                if (!receiving_mqtt) {
                    if (strstr(line, "+CMQTTRXSTART:") != NULL) {
                        // Bắt đầu nhận chuỗi MQTT URC
                        receiving_mqtt = true;
                        mqtt_rx_state = 0;
                        mqtt_payload[0] = '\0';
                    } else if (len > 2) {
                        // Dòng phản hồi bình thường của 1 lệnh AT
                        sim_line_t l;
                        strncpy(l.text, line, sizeof(l.text) - 1);
                        l.text[sizeof(l.text) - 1] = '\0';
                        xQueueSend(resp_queue, &l, 0); // non-blocking
                    }
                } else {
                    // Đang thu thập các dòng của URC MQTT
                    if (strstr(line, "+CMQTTRXTOPIC:") != NULL) {
                        mqtt_rx_state = 1; // Dòng tiếp theo sẽ chứa Topic
                    } else if (strstr(line, "+CMQTTRXPAYLOAD:") != NULL) {
                        mqtt_rx_state = 2; // Dòng tiếp theo sẽ chứa Payload
                    } else if (strstr(line, "+CMQTTRXEND:") != NULL) {
                        // Đã nhận đủ tin nhắn, gọi hàm xử lý
                        printf("\n-----------------------------------------\n");
                        printf("[KẾT QUẢ] NHẬN ĐƯỢC LỆNH TỪ BROKER:\n%s\n", mqtt_payload);
                        printf("-----------------------------------------\n\n");
                        
                        mqtt_message_received(MQTT_TOPIC_COMMAND, mqtt_payload);
                        
                        // Reset state
                        receiving_mqtt = false;
                        mqtt_rx_state = 0;
                    } else {
                        // Loại bỏ \r\n ở cuối dòng để lấy nội dung sạch
                        char clean_line[256];
                        strncpy(clean_line, line, sizeof(clean_line));
                        for (int i = strlen(clean_line) - 1; i >= 0; i--) {
                            if (clean_line[i] == '\r' || clean_line[i] == '\n') {
                                clean_line[i] = '\0';
                            } else {
                                break;
                            }
                        }

                        if (strlen(clean_line) > 0) {
                            if (mqtt_rx_state == 1) {
                                // Đây là chuỗi Topic thực tế
                                mqtt_rx_state = 0;
                            } else if (mqtt_rx_state == 2) {
                                // Đây là chuỗi Payload thực tế
                                strncpy(mqtt_payload, clean_line, sizeof(mqtt_payload) - 1);
                                mqtt_payload[sizeof(mqtt_payload) - 1] = '\0';
                                mqtt_rx_state = 0;
                            }
                        }
                    }
                }
                
                len = 0;
                line[0] = '\0';
            }
        }
    }
}

// =====================================================================
// LỚP 4: API GỬI LỆNH AT - đọc từ resp_queue, KHÔNG đụng UART trực tiếp
// =====================================================================
static bool sim7680_read_response(char *response, size_t resp_size, uint32_t timeout_ms)
{
    if (response && resp_size > 0) {
        response[0] = '\0';
    }

    if (resp_queue == NULL) {
        // sim7680_init() thất bại lúc tạo queue (het heap) - tránh crash,
        // trả về false ngay thay vì gọi xQueueReceive() trên handle NULL.
        return false;
    }

    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
    sim_line_t l;

    while (true) {
        int64_t remain_us = absolute_time_diff_us(get_absolute_time(), deadline);
        if (remain_us <= 0) break;

        TickType_t remain_ticks = pdMS_TO_TICKS((uint32_t)(remain_us / 1000) + 1);

        if (xQueueReceive(resp_queue, &l, remain_ticks) == pdTRUE) {
            if (response) {
                size_t used = strlen(response);
                if (used < resp_size - 1) {
                    strncat(response, l.text, resp_size - used - 1);
                }
            }
            if (strstr(l.text, "OK") != NULL)    return true;
            if (strstr(l.text, "ERROR") != NULL) return false;
            if (strcmp(l.text, ">") == 0)        return true; // dùng cho sim7680_wait_prompt()
        }
    }

    // Hết thời gian chờ nhưng có thể đã nhận được OK ở giữa các dòng đã gom được
    if (response && strstr(response, "OK") != NULL) {
        return true;
    }
    return false;
}

// =====================================================================
// LỚP 5: HÀM PUBLIC (giữ nguyên chữ ký so với bản cũ)
// =====================================================================
void sim7680_init(void)
{
    uart_init(SIM7680_UART, SIM7680_BAUDRATE);

    gpio_set_function(SIM7680_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(SIM7680_UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_hw_flow(SIM7680_UART, false, false);
    uart_set_format(SIM7680_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(SIM7680_UART, true);

    // Chỉ tạo queue nhỏ (~400 byte) - phải tạo TRƯỚC khi bật ngắt UART,
    // tránh ISR chạy trước khi đối tượng FreeRTOS này sẵn sàng.
    resp_queue = xQueueCreate(4, sizeof(sim_line_t));
    if (resp_queue == NULL) {
        // Heap không đủ - báo lỗi rõ ràng và DỪNG LẠI ở đây, không bật
        // interrupt/tạo task, để tránh crash (xQueueSend vào NULL handle).
        // Hệ thống vẫn chạy các task khác bình thường, chỉ riêng MQTT sẽ
        // luôn timeout khi gửi lệnh AT (dễ nhận biết qua log) thay vì đứng
        // hình không rõ nguyên nhân như trước.
        printf("[SIM] LOI: Khong the tao resp_queue (het heap?). MQTT se khong hoat dong.\n");
        return;
    }

    int uart_irq = (SIM7680_UART == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(uart_irq, sim7680_uart_isr);
    irq_set_enabled(uart_irq, true);
    uart_set_irq_enables(SIM7680_UART, true, false); // chỉ bật RX interrupt, không cần TX interrupt

    // Stack giảm còn 384 word (1536 byte) thay vì 1024 word (4096 byte) -
    // task này chỉ so sánh chuỗi + gọi xQueueSend + mqtt_message_received(),
    // không cần nhiều stack. Nếu sau này thêm xử lý nặng hơn trong
    // mqtt_message_received(), có thể phải tăng lại số này.
    BaseType_t ok = xTaskCreate(sim7680_rx_task, "SIM_RX", 384, NULL,
                                 configMAX_PRIORITIES - 2, NULL);
    if (ok != pdPASS) {
        printf("[SIM] LOI: Khong the tao SIM_RX task (het heap?). MQTT se khong hoat dong.\n");
    }
}

bool sim7680_send_cmd(const char *cmd, char *response, size_t resp_size, uint32_t timeout_ms)
{
    // KHÔNG còn flush_rx() nữa: resp_queue chỉ chứa các dòng phát sinh
    // SAU khi gửi lệnh này, vì URC đã bị RX task chặn và xử lý riêng rồi.
    uart_puts(SIM7680_UART, cmd);
    uart_puts(SIM7680_UART, "\r\n");

    return sim7680_read_response(response, resp_size, timeout_ms);
}

bool sim7680_wait_prompt(uint32_t timeout_ms)
{
    return sim7680_read_response(NULL, 0, timeout_ms);
}

bool sim7680_read_ok(char *response, size_t resp_size, uint32_t timeout_ms)
{
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
        vTaskDelay(pdMS_TO_TICKS(500));
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

bool sim7680_get_gprs_attach(bool *attached)
{
    char resp[64];
    if (!sim7680_send_cmd("AT+CGATT?", resp, sizeof(resp), 1000)) {
        return false;
    }

    char *p = strstr(resp, "+CGATT:");
    if (!p) {
        return false;
    }
    int value = 0;
    if (sscanf(p, "+CGATT: %d", &value) != 1) {
        return false;
    }
    *attached = (value == 1);
    return true;
}

bool sim7680_wait_network_ready(uint32_t timeout_ms)
{
    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
    while (absolute_time_diff_us(get_absolute_time(), deadline) > 0) {
        int status = 0;
        bool attached = false;
        if (sim7680_get_network_status(&status) && status >= 1 && status <= 5 &&
            sim7680_get_gprs_attach(&attached) && attached) {
            printf("[SIM] Network registered and attached (CREG=%d, CGATT=1)\n", status);
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    return false;
}

bool sim7680_check_sim(bool *ready)
{
    char resp[64] = {0};
    *ready = false;

    if (!sim7680_send_cmd("AT+CPIN?", resp, sizeof(resp), 3000)) {
        printf("[SIM] AT+CPIN? gui that bai. Resp: %s\n", resp);
        return false;
    }

    printf("[SIM DEBUG] AT+CPIN? response: [%s]\n", resp);

    if (strstr(resp, "+CPIN: READY") != NULL) {
        *ready = true;
        return true;
    }
    if (strstr(resp, "+CPIN: SIM PIN") != NULL) {
        printf("[SIM] SIM dang yeu cau PIN!\n");
    }

    return false;
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
#include "sim7080g.h"
#include <stdio.h>

void sim_init() {
    // Khởi tạo bộ cứng UART1
    uart_init(SIM_UART_PORT, SIM_BAUD_RATE);
    gpio_set_function(PIN_SIM_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_SIM_RX, GPIO_FUNC_UART);

    // Cấu hình chân PWRKEY để kích nguồn
    gpio_init(PIN_SIM_PWR);
    gpio_set_dir(PIN_SIM_PWR, GPIO_OUT);
    
    // Xung kích nguồn: Kéo xuống LOW 1.5 giây rồi trả về HIGH
    printf("[SIM] Dang kich nguon SIM7080G...\n");
    gpio_put(PIN_SIM_PWR, 0);
    sleep_ms(1500);
    gpio_put(PIN_SIM_PWR, 1);
    printf("[SIM] Kich nguon xong. Cho module on dinh...\n");
    sleep_ms(3000); 
}

void sim_send_at(const char* cmd) {
    printf("[SIM] Sent: %s", cmd);
    uart_puts(SIM_UART_PORT, cmd);
    
    // Chờ 500ms để module xử lý và phản hồi dữ liệu
    sleep_ms(500);
    
    // Đọc phản hồi trả về từ đường RX
    if (uart_is_readable(SIM_UART_PORT)) {
        printf("[SIM] Response:\n");
        while (uart_is_readable(SIM_UART_PORT)) {
            char ch = uart_getc(SIM_UART_PORT);
            putchar(ch); 
        }
        printf("\n-------------------------\n");
    } else {
        printf("[SIM] No response! Kiem tra day ket noi hoac nguon.\n-------------------------\n");
    }
}
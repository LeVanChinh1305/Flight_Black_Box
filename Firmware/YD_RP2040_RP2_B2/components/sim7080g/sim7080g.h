#ifndef SIM7080G_H
#define SIM7080G_H

#include "pico/stdlib.h"
#include "hardware/uart.h"

// Định nghĩa phần cứng UART cho SIM7080G
#define SIM_UART_PORT uart1
#define SIM_BAUD_RATE 115200
#define PIN_SIM_TX    4
#define PIN_SIM_RX    5
#define PIN_SIM_PWR   2

// Khai báo các hàm giao tiếp với SIM
// hàm khởi tạo 
void sim_init();
// hàm 
void sim_send_at(const char* cmd);

#endif // SIM7080G_H
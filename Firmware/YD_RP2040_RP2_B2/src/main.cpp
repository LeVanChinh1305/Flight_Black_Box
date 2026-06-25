#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"

#include "components/sim7080g/sim7080g.h"


int main() {
    // Khởi tạo clock hệ thống và STDIO USB
    set_sys_clock_khz(125000, true);
    sleep_ms(500);
    stdio_init_all();
    sleep_ms(2000);

    // Gọi hàm khởi tạo SIM từ file sim7080.cpp
    sim_init();

    
    printf("He thong Hop den: Khoi tao cac ngoai vi hoan tat!\n");

    while (true) {
        // Kiểm tra kết nối định kỳ bằng lệnh AT cơ bản
        sim_send_at("AT\r\n");
        sim_send_at("AT+CPIN?\r\n");
        sim_send_at("AT+CEREG?\r\n");

        sleep_ms(5000);
    }
}

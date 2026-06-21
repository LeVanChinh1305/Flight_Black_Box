/*
 * main.c
 *
 * MSP430F5529_black_box — nhận dữ liệu UART từ STM32, chuyển tiếp qua LoRa SX1278.
 *
 * Luồng dữ liệu:
 *   STM32 --UART(P3.4)--> UART_Receiver --[UartFrame_t]--> LoRa_SX1278 --433MHz--> ESP32 Boss
 *
 * Vai trò main.c:
 *   1. Cấu hình clock hệ thống (DCO → SMCLK 1MHz)
 *   2. Tắt watchdog
 *   3. Gọi init UART_Receiver và LoRa_SX1278
 *   4. Vòng lặp chính: gọi UART_Receiver_Process() liên tục
 *      (callback UART_Receiver_OnFrameReceived() ở dưới sẽ tự gửi LoRa khi có frame)
 */

#include <msp430.h>
#include "App_Types.h"
#include "UART_Receiver/UART_Receiver.h"
#include "LoRa_SX1278/LoRa_SX1278.h"

/* ===========================================================================
 * CẤU HÌNH CLOCK HỆ THỐNG
 * =========================================================================== */

/**
 * @brief  Cấu hình DCO chạy ở 1 MHz, dùng làm MCLK và SMCLK.
 *         1 MHz đủ cho UART 115200 baud và SPI 125kHz mà không cần
 *         chỉnh sửa nhiều thanh ghi clock phức tạp.
 */
static void Clock_Init(void)
{
    /* Dùng tần số DCO mặc định sau reset (~1.045 MHz, calibrated REFO source)
     * MSP430F5529 mặc định MCLK = SMCLK = DCO ~1MHz sau khi reset,
     * nên không cần cấu hình thêm cho yêu cầu baud/SPI hiện tại. */

    /* Nếu cần chính xác hơn, có thể cấu hình UCS module ở đây.
     * Để đơn giản và ổn định, giữ nguyên DCO mặc định ~1MHz. */
}

/* ===========================================================================
 * CALLBACK — ĐƯỢC GỌI KHI UART_Receiver NHẬN ĐỦ 1 FRAME HỢP LỆ
 * =========================================================================== */

void UART_Receiver_OnFrameReceived(const UartFrame_t *frame)
{
    /* Chuyển tiếp ngay frame vừa nhận qua LoRa.
     * LoRa_SX1278_Send() là hàm blocking (~vài chục ms), chấp nhận được
     * vì tốc độ frame từ STM32 là 100Hz nhưng LoRa không thể theo kịp tốc độ đó —
     * đây là điểm nghẽn tự nhiên của hệ thống (LoRa bandwidth thấp hơn nhiều UART). */
    LoRa_SX1278_Send(frame);
}

/* ===========================================================================
 * MAIN
 * =========================================================================== */

int main(void)
{
    /* --- Tắt watchdog ngay từ đầu --- */
    WDTCTL = WDTPW | WDTHOLD;

    /* --- Cấu hình clock hệ thống --- */
    Clock_Init();

    /* --- Khởi tạo các module --- */
    UART_Receiver_Init();

    if (LoRa_SX1278_Init() != 0) {
        /* SX1278 không phản hồi đúng — có thể sai dây SPI hoặc module hỏng.
         * Dừng tại đây để dễ debug bằng breakpoint thay vì chạy tiếp với LoRa lỗi. */
        while (1) {
            __no_operation();
        }
    }

    /* --- Bật ngắt toàn cục (cần cho UART_Receiver ISR hoạt động) --- */
    __bis_SR_register(GIE);

    /* --- Vòng lặp chính --- */
    while (1) {
        UART_Receiver_Process();
        /* UART_Receiver_Process() sẽ tự gọi callback OnFrameReceived()
         * mỗi khi tìm thấy 1 frame hợp lệ trong buffer. */
    }
}
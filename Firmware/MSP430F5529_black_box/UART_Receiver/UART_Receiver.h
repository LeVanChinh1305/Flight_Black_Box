#ifndef UART_RECEIVER_H_
#define UART_RECEIVER_H_

/*
 * UART_Receiver.h
 *
 * Module nhận dữ liệu UART từ STM32 và parse thành UartFrame_t.
 *
 * Phần cứng dùng:
 *   USCI_A0  —  P3.4 = UCA0RXD (nhận từ PA9 TX của STM32)
 *               P3.3 = UCA0TXD (không dùng, để sẵn)
 *   Baud rate: 115200, 8N1 (khớp với cấu hình UART1 bên STM32)
 *
 * Cách hoạt động:
 *   - Ngắt RX (UCA0RXIFG) được bật, mỗi byte nhận vào sẽ gọi ISR
 *   - ISR đẩy byte vào ring buffer nội bộ
 *   - UART_Receiver_Process() được gọi trong vòng lặp main để
 *     tìm header, parse frame, verify checksum
 *
 * Sử dụng:
 *   1. UART_Receiver_Init()  — gọi 1 lần trong main(), sau khi cấu hình clock
 *   2. Trong vòng lặp chính, gọi UART_Receiver_Process() liên tục
 *   3. Nếu có frame hợp lệ, callback UART_Receiver_OnFrameReceived() được gọi
 */

#include "../App_Types.h"
#include <stdint.h>

/* ===========================================================================
 * CẤU HÌNH
 * =========================================================================== */

/* SMCLK dùng làm clock nguồn cho UCA0 — phải khớp với cấu hình clock trong main.c */
#define UART_RX_SMCLK_HZ      1000000UL   /* 1 MHz */
#define UART_RX_BAUD          115200UL

/* Kích thước ring buffer nội bộ (byte) */
#define UART_RX_BUF_SIZE       128

/* ===========================================================================
 * API
 * =========================================================================== */

/**
 * @brief  Khởi tạo USCI_A0 ở chế độ UART, baud 115200, bật ngắt RX.
 *         Gọi 1 lần trong main(), sau khi cấu hình clock hệ thống.
 */
void UART_Receiver_Init(void);

/**
 * @brief  Xử lý dữ liệu trong ring buffer — tìm header, parse frame,
 *         verify checksum và tail. Gọi liên tục trong vòng lặp main.
 *         Khi tìm thấy frame hợp lệ, hàm sẽ gọi callback
 *         UART_Receiver_OnFrameReceived().
 */
void UART_Receiver_Process(void);

/**
 * @brief  Callback được gọi khi UART_Receiver_Process() tìm thấy 1 frame
 *         hợp lệ (đúng header, tail, checksum).
 *         Định nghĩa hàm này ở main.c hoặc module khác (vd: LoRa_SX1278)
 *         để xử lý dữ liệu — ví dụ gửi tiếp qua LoRa.
 *
 * @param  frame  Con trỏ tới frame đã parse xong (nằm trong buffer nội bộ,
 *                chỉ hợp lệ trong thời gian gọi callback — nếu cần giữ lại
 *                lâu hơn, hãy copy ra biến khác).
 */
void UART_Receiver_OnFrameReceived(const UartFrame_t *frame);

#endif /* UART_RECEIVER_H_ */
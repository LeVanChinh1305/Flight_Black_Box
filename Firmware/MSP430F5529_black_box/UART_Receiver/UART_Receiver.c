#include "UART_Receiver.h"
#include <msp430.h>
#include <string.h>

/* ===========================================================================
 * RING BUFFER NỘI BỘ
 * =========================================================================== */

static volatile uint8_t  s_rx_buf[UART_RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0;   /* Vị trí ISR ghi vào */
static volatile uint16_t s_rx_tail = 0;   /* Vị trí Process() đọc ra */

/* ===========================================================================
 * HÀM NỘI BỘ
 * =========================================================================== */

/**
 * @brief  Số byte hiện có trong ring buffer (chưa xử lý).
 */
static uint16_t rx_buf_count(void)
{
    return (uint16_t)(s_rx_head - s_rx_tail) & (UART_RX_BUF_SIZE - 1);
}

/**
 * @brief  Đọc byte tại vị trí offset tính từ s_rx_tail, không tiêu thụ.
 */
static uint8_t rx_buf_peek(uint16_t offset)
{
    return s_rx_buf[(s_rx_tail + offset) & (UART_RX_BUF_SIZE - 1)];
}

/**
 * @brief  Bỏ qua (tiêu thụ) n byte đầu của ring buffer.
 */
static void rx_buf_advance(uint16_t n)
{
    s_rx_tail = (s_rx_tail + n) & (UART_RX_BUF_SIZE - 1);
}

/**
 * @brief  Tính checksum XOR của payload trong frame (38 byte,
 *         từ sau header đến trước checksum), khớp với cách tính bên STM32.
 */
static uint8_t calc_checksum(const uint8_t *frame_bytes)
{
    uint8_t cs = 0;
    uint16_t payload_start = 2;                          /* bỏ qua header[0..1] */
    uint16_t payload_end   = FRAME_SIZE - 2;              /* bỏ checksum + tail */
    uint16_t i;

    for (i = payload_start; i < payload_end; i++) {
        cs ^= frame_bytes[i];
    }
    return cs;
}

/* ===========================================================================
 * API CÔNG KHAI
 * =========================================================================== */

void UART_Receiver_Init(void)
{
    /* --- Cấu hình GPIO P3.3/P3.4 sang chức năng UCA0 (xem pinout BoosterPack) --- */
    P3SEL |= BIT3 | BIT4;     /* P3.3 = UCA0TXD, P3.4 = UCA0RXD */

    /* --- Reset USCI_A0 trước khi cấu hình --- */
    UCA0CTL1 |= UCSWRST;

    /* --- Chọn clock nguồn: SMCLK --- */
    UCA0CTL1 |= UCSSEL__SMCLK;

    /*
     * --- Tính toán thanh ghi baud rate ---
     * Với SMCLK = 1MHz, baud = 115200:
     *   N = SMCLK / baud = 1000000 / 115200 = 8.68
     *   UCA0BR0/1 = floor(N) = 8
     *   UCBRSx tra bảng User's Guide (Table 30-4, gần đúng cho phần dư 0.68 -> giá trị 0x11)
     * (Giá trị chuẩn theo TI User's Guide cho 1MHz/115200)
     */
    UCA0BR0 = 8;
    UCA0BR1 = 0;
    UCA0MCTL = UCBRS_6 | UCBRF_0;   /* UCOS16 = 0 (mặc định, không cần set rõ) */

    /* --- Thoát reset, USCI_A0 bắt đầu hoạt động --- */
    UCA0CTL1 &= ~UCSWRST;

    /* --- Bật ngắt nhận (RX) --- */
    UCA0IE |= UCRXIE;

    /* --- Reset ring buffer --- */
    s_rx_head = 0;
    s_rx_tail = 0;
}

void UART_Receiver_Process(void)
{
    /* Cần tối thiểu FRAME_SIZE byte mới có thể có 1 frame đầy đủ */
    while (rx_buf_count() >= FRAME_SIZE) {

        /* --- Tìm header 0xAA 0x55 --- */
        if (rx_buf_peek(0) != FRAME_HEADER_0 ||
            rx_buf_peek(1) != FRAME_HEADER_1) {
            /* Không phải header — bỏ 1 byte rác, thử lại */
            rx_buf_advance(1);
            continue;
        }

        /* --- Copy FRAME_SIZE byte ra buffer tạm để parse an toàn --- */
        uint8_t  frame_bytes[FRAME_SIZE];
        uint16_t i;
        for (i = 0; i < FRAME_SIZE; i++) {
            frame_bytes[i] = rx_buf_peek(i);
        }

        /* --- Kiểm tra tail --- */
        if (frame_bytes[FRAME_SIZE - 1] != FRAME_TAIL) {
            /* Tail sai — header giả, bỏ 1 byte và tìm lại */
            rx_buf_advance(1);
            continue;
        }

        /* --- Verify checksum --- */
        uint8_t expected_cs = frame_bytes[FRAME_SIZE - 2];
        uint8_t actual_cs   = calc_checksum(frame_bytes);

        /* Đã xử lý xong frame này (dù đúng hay sai) — tiêu thụ khỏi buffer */
        rx_buf_advance(FRAME_SIZE);

        if (actual_cs == expected_cs) {
            /* Frame hợp lệ — ép kiểu sang UartFrame_t và gọi callback */
            UartFrame_t frame;
            memcpy(&frame, frame_bytes, FRAME_SIZE);
            UART_Receiver_OnFrameReceived(&frame);
        }
        /* Nếu checksum sai — âm thầm bỏ qua, chờ frame tiếp theo */
    }
}

/* ===========================================================================
 * ISR — NGẮT NHẬN UART
 * =========================================================================== */

#pragma vector = USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    switch (__even_in_range(UCA0IV, 4)) {
        case 0:  break;                       /* No interrupt */
        case 2:                                /* UCRXIFG — byte mới đã nhận */
        {
            uint8_t rx_byte = UCA0RXBUF;       /* Đọc tự động clear cờ ngắt */
            uint16_t next_head = (s_rx_head + 1) & (UART_RX_BUF_SIZE - 1);

            /* Nếu buffer đầy thì bỏ byte mới (drop) để không ghi đè dữ liệu chưa xử lý */
            if (next_head != s_rx_tail) {
                s_rx_buf[s_rx_head] = rx_byte;
                s_rx_head = next_head;
            }
            break;
        }
        case 4:  break;                       /* UCTXIFG — không dùng vì không gửi */
        default: break;
    }
}
#ifndef LORA_SX1278_H_
#define LORA_SX1278_H_

/*
 * LoRa_SX1278.h
 *
 * Driver SPI cho module LoRa SX1278 — dùng để gửi UartFrame_t
 * nhận được từ STM32 (qua UART_Receiver) lên ESP32 (Boss) bằng sóng vô tuyến.
 *
 * Phần cứng dùng (USCI_B0, theo BoosterPack pinout MSP-EXP430F5529LP):
 *   P3.0 = UCB0SIMO  → SX1278 MOSI
 *   P3.1 = UCB0SOMI  ← SX1278 MISO
 *   P2.6 = UCB0SCL   → SX1278 SCK     (UCB0SCL dùng làm SPI clock ở mode 4-wire)
 *   P2.2 = GPIO      → SX1278 NSS/CS  (chip select phần mềm)
 *   P7.4 = GPIO      → SX1278 RST
 *   P2.0 = GPIO      ← SX1278 DIO0    (TxDone interrupt, đọc polling)
 *
 * Cấu hình LoRa: 433 MHz, SF7, BW125kHz, CR4/5, payload cố định 42 byte.
 */

#include "../App_Types.h"
#include <stdint.h>

/* ===========================================================================
 * API
 * =========================================================================== */

/**
 * @brief  Khởi tạo SPI (USCI_B0), GPIO điều khiển (CS, RST, DIO0),
 *         reset và cấu hình thanh ghi SX1278 sang chế độ LoRa TX-ready.
 *         Gọi 1 lần trong main(), sau khi cấu hình clock hệ thống.
 *
 * @retval 0  = OK
 * @retval -1 = Không đọc được version register (SX1278 không phản hồi — kiểm tra dây)
 */
int LoRa_SX1278_Init(void);

/**
 * @brief  Gửi 1 frame qua LoRa (blocking — chờ đến khi TxDone hoặc timeout).
 * @param  frame  Con trỏ tới UartFrame_t cần gửi (42 byte).
 * @retval 0  = OK
 * @retval -1 = Timeout chờ TxDone
 */
int LoRa_SX1278_Send(const UartFrame_t *frame);

#endif /* LORA_SX1278_H_ */
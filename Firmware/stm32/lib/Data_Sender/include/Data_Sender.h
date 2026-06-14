#ifndef DATA_SENDER_H
#define DATA_SENDER_H

/**
 * @file    Data_Sender.h
 * @brief   Module đóng gói và gửi ProcessedData_t sang ESP32 qua UART.
 *
 * Binary frame protocol:
 * ┌────────┬────────┬──────────────────────────────────────────────────┬──────────┬──────┐
 * │ 0xAA   │ 0x55   │ Payload (38 byte)                                │ Checksum │ 0xFF │
 * │ Header │ Header │ timestamp | acc_xyz | gyr_xyz | temp | pitch|roll│ XOR      │ Tail │
 * └────────┴────────┴──────────────────────────────────────────────────┴──────────┴──────┘
 *
 * Tổng frame: 2 + 38 + 1 + 1 = 42 byte
 * Checksum  : XOR của toàn bộ 38 byte payload (không gồm header và tail)
 *
 * Bên ESP32 nhận:
 *   1. Quét đến 0xAA 0x55
 *   2. Đọc tiếp 39 byte (payload + checksum)
 *   3. Verify checksum XOR
 *   4. Kiểm tra tail 0xFF
 */

#include "app_types.h"
#include "stm32f1xx_hal.h"

// ===========================================================================
// CẤU HÌNH
// ===========================================================================

/** UART handle gửi sang ESP32 (UART2: PA2 TX, PA3 RX) */
extern UART_HandleTypeDef huart2;

/** Timeout gửi UART (ms) — đủ cho 42 byte @ 115200 baud ≈ 4ms, để 50ms an toàn */
#define DATA_SENDER_UART_TIMEOUT_MS   50U

/** Header và tail của frame */
#define DATA_SENDER_FRAME_HEADER_0    0xAAU
#define DATA_SENDER_FRAME_HEADER_1    0x55U
#define DATA_SENDER_FRAME_TAIL        0xFFU

// ===========================================================================
// FRAME STRUCTURE
// ===========================================================================

/**
 * @brief  Binary frame gửi sang ESP32.
 *         __packed__ đảm bảo không có padding byte giữa các trường.
 */
typedef struct __attribute__((packed)) {
    uint8_t  header[2];       // [0xAA, 0x55]
    uint32_t timestamp_ms;    // Thời điểm đo [ms]
    float    acc_x_g;         // [g]
    float    acc_y_g;         // [g]
    float    acc_z_g;         // [g]
    float    gyr_x_dps;       // [deg/s]
    float    gyr_y_dps;       // [deg/s]
    float    gyr_z_dps;       // [deg/s]
    float    temp_c;          // [°C]
    float    pitch_deg;       // [°]
    float    roll_deg;        // [°]
    uint8_t  checksum;        // XOR của 38 byte payload
    uint8_t  tail;            // 0xFF
} UartFrame_t;

// ===========================================================================
// API
// ===========================================================================

/**
 * @brief  Khởi tạo module Data_Sender.
 *         Gọi trước vTaskStartScheduler().
 * @retval 0 = OK
 */
int Data_Sender_Init(void);

/**
 * @brief  FreeRTOS task function — truyền vào xTaskCreate().
 * @param  pvParameters  Không dùng, truyền NULL.
 */
void Data_Sender_Task(void *pvParameters);

#endif /* DATA_SENDER_H */
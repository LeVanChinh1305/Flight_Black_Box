#ifndef DATA_SENDER_H
#define DATA_SENDER_H

/**
 * @file    Data_Sender.h
 * @brief   Đóng gói và gửi ProcessedData_t sang ESP32 qua UART1.
 *
 * Binary frame protocol:
 * ┌────────┬────────┬────────────────────────────────────────────────┬──────────┬──────┐
 * │ 0xAA   │ 0x55   │ Payload (38 byte)                              │ Checksum │ 0xFF │
 * │ Header │ Header │ timestamp|acc_xyz|gyr_xyz|temp|pitch|roll      │ XOR      │ Tail │
 * └────────┴────────┴────────────────────────────────────────────────┴──────────┴──────┘
 *
 * Tổng frame: 2 + 38 + 1 + 1 = 42 byte
 * Checksum  : XOR của 38 byte payload (không gồm header và tail)
 */

#include "app_types.h"
#include "stm32f1xx_hal.h"

// UART1 → ESP32
extern UART_HandleTypeDef huart1;

// Timeout gửi: 42 byte @ 115200 baud ≈ 4ms, để 50ms an toàn
#define DATA_SENDER_UART_TIMEOUT_MS   50U

#define DATA_SENDER_FRAME_HEADER_0    0xAAU
#define DATA_SENDER_FRAME_HEADER_1    0x55U
#define DATA_SENDER_FRAME_TAIL        0xFFU

typedef struct __attribute__((packed)) {
    uint8_t  header[2];
    uint32_t timestamp_ms;
    float    acc_x_g;
    float    acc_y_g;
    float    acc_z_g;
    float    gyr_x_dps;
    float    gyr_y_dps;
    float    gyr_z_dps;
    float    temp_c;
    float    pitch_deg;
    float    roll_deg;
    uint8_t  checksum;
    uint8_t  tail;
} UartFrame_t;

int  Data_Sender_Init(void);
void Data_Sender_Task(void *pvParameters);

#endif /* DATA_SENDER_H */
#ifndef NEO6M_H
#define NEO6M_H

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// -------------------- Cấu hình chân UART mặc định --------------------
#define NEO6M_UART_PORT       uart0
#define NEO6M_PIN_TX          0   // TX0 của Pico → RX của GPS
#define NEO6M_PIN_RX          1   // RX0 của Pico ← TX của GPS
#define NEO6M_BAUDRATE        9600
#define NEO6M_UART_TIMEOUT_MS 2500

// -------------------- Các định nghĩa giao thức NMEA --------------------
#define NEO6M_NMEA_MAX_LEN  100
#define NEO6M_NMEA_GGA      "$GPGGA"
#define NEO6M_NMEA_RMC      "$GPRMC"
#define NEO6M_NMEA_START    '$'
#define NEO6M_NMEA_END_CR   '\r'
#define NEO6M_NMEA_END_LF   '\n'

// -------------------- Các cấu trúc dữ liệu --------------------
typedef enum {
    NEO6M_FIX_INVALID = 0,
    NEO6M_FIX_GPS     = 1,
    NEO6M_FIX_DGPS    = 2,
} neo6m_fix_quality_t;

typedef struct {
    float    latitude;          // Vĩ độ (độ thập phân)
    float    longitude;         // Kinh độ (độ thập phân)
    float    altitude_m;        // Độ cao (mét)

    neo6m_fix_quality_t fix_quality;
    uint8_t  satellites;        // Số vệ tinh
    float    hdop;              // Sai số hình học HDOP

    float    speed_knots;       // Tốc độ (knot)
    float    speed_kmh;         // Tốc độ (km/h)
    float    course_deg;        // Hướng chuyển động (độ)

    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;

    uint8_t  day;
    uint8_t  month;
    uint16_t year;

    bool     is_valid;          // Trạng thái dữ liệu hợp lệ
} neo6m_data_t;

typedef struct {
    uart_inst_t *handle_uart;
    uint         pin_tx;
    uint         pin_rx;
    uint32_t     baudrate;
    bool         is_initialized;
    char         nmea_buf[NEO6M_NMEA_MAX_LEN];
    uint8_t      buf_idx;
} neo6m_dev_t;

typedef enum {
    NEO6M_OK    = 0,
    NEO6M_ERROR = 1,
} NEO6M_Status;

// -------------------- API Công khai --------------------
NEO6M_Status NEO6M_Init(neo6m_dev_t *dev, uart_inst_t *handle_uart, uint pin_tx, uint pin_rx, uint32_t baudrate);
NEO6M_Status NEO6M_ReadLine(neo6m_dev_t *dev);
NEO6M_Status NEO6M_Update(neo6m_dev_t *dev, neo6m_data_t *data);
NEO6M_Status NEO6M_ParseGGA(const char *sentence, neo6m_data_t *data);
NEO6M_Status NEO6M_ParseRMC(const char *sentence, neo6m_data_t *data);
bool         NEO6M_ValidateChecksum(const char *sentence);

#ifdef __cplusplus
}
#endif

#endif // NEO6M_H
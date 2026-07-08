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
#define NEO6M_BAUDRATE        9600   // Baud mac dinh khi module NEO-6M moi bat dien (dung de mo ket noi lan dau)

// Baud muc tieu se duoc cau hinh (qua UBX-CFG-PRT) ngay sau khi ket noi o NEO6M_BAUDRATE.
// Tang baud giup giam dang ke thoi gian truyen 1 cau NMEA (~70 ky tu):
//   9600 baud  -> ~73 ms/cau
//   38400 baud -> ~18 ms/cau  (nhanh hon ~4 lan)
// Neu module/day cap khong on dinh o baud cao, co the doi ve 19200 hoac tra ve NEO6M_BAUDRATE
// (bo qua buoc doi baud) bang cach sua dinh nghia nay.
#define NEO6M_TARGET_BAUDRATE  9600

#define NEO6M_UART_TIMEOUT_MS 300 

// Timeout cho ReadLine noi bo:
// - SEARCH: thoi gian toi da cho de tim ky tu bat dau '$'
// - READ:   thoi gian toi da (tinh tu khi thay '$') de doc het 1 dong den '\n'
// Cac gia tri nay duoc tinh du cho ca truong hop dang chay o NEO6M_BAUDRATE (9600, luc
// chua doi baud xong); sau khi da chuyen sang NEO6M_TARGET_BAUDRATE, thoi gian thuc te
// de doc het 1 cau se nho hon nhieu so voi cac moc timeout nay (chi la gioi han an toan).
#define NEO6M_LINE_SEARCH_TIMEOUT_MS 60
#define NEO6M_LINE_READ_TIMEOUT_MS   80

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
    NEO6M_OK      = 0,
    NEO6M_ERROR   = 1,
    NEO6M_UNKNOWN = 2,   // Doc thanh cong 1 dong NMEA nhung khong phai GGA/RMC (chua parse)
    NEO6M_TIMEOUT = 3,
} NEO6M_Status;

// -------------------- API Công khai --------------------
NEO6M_Status NEO6M_Init(neo6m_dev_t *dev, uart_inst_t *handle_uart, uint pin_tx, uint pin_rx, uint32_t baudrate);
NEO6M_Status NEO6M_ReadLine(neo6m_dev_t *dev); // Đọc 1 dòng NMEA từ UART, lưu vào nmea_buf trong dev. Trả về trạng thái đọc dữ liệu (NEO6M_OK nếu thành công, NEO6M_ERROR nếu có lỗi, NEO6M_UNKNOWN nếu không đọc được dòng hợp lệ)
NEO6M_Status NEO6M_Update(neo6m_dev_t *dev, neo6m_data_t *data); // Đọc dữ liệu GPS mới nhất từ module NEO-6M, trả về trạng thái đọc dữ liệu (NEO6M_OK nếu thành công, NEO6M_ERROR nếu có lỗi, NEO6M_UNKNOWN nếu dòng NMEA không phải GGA/RMC)
NEO6M_Status NEO6M_Update_WithTimeout(neo6m_dev_t *dev, neo6m_data_t *data, uint32_t timeout_ms); // Đọc dữ liệu GPS mới nhất từ module NEO-6M, trả về trạng thái đọc dữ liệu (NEO6M_OK nếu thành công, NEO6M_ERROR nếu có lỗi, NEO6M_UNKNOWN nếu dòng NMEA không phải GGA/RMC)
NEO6M_Status NEO6M_ParseGGA(const char *sentence, neo6m_data_t *data); // Hàm phân tích cú pháp dòng NMEA GGA, trích xuất thông tin vị trí, độ cao, số vệ tinh và chất lượng tín hiệu GPS
NEO6M_Status NEO6M_ParseRMC(const char *sentence, neo6m_data_t *data); // Hàm phân tích cú pháp dòng NMEA RMC, trích xuất thông tin tốc độ, hướng chuyển động và thời gian
bool         NEO6M_ValidateChecksum(const char *sentence); // Hàm kiểm tra tính hợp lệ của checksum trong dòng NMEA, trả về true nếu checksum hợp lệ, false nếu không hợp lệ

#ifdef __cplusplus
}
#endif

#endif // NEO6M_H
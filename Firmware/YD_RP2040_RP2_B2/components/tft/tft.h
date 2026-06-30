#ifndef TFT_H
#define TFT_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// --------------------các định nghĩa về cấu hình SPI & chân điều khiển -------------------

// Cấu hình bộ ngoại vi SPI dùng chung cho màn hình TFT (ILI9341 - 2.8" SPI 240x320)
#define TFT_SPI_PORT     spi1     // SPI 1 - Chung chân
#define TFT_PIN_SDO      12       // MISO1
#define TFT_PIN_SDI      15       // MOSI1
#define TFT_PIN_SCL      14       // SCK1
#define TFT_PIN_CS       9        // Chip Select (Output)
#define TFT_PIN_RST      8        // Reset       (Output)
#define TFT_PIN_DC       7        // Data/Command(Output)
#define TFT_PIN_LED      6        // Đèn nền     (Output)

#define TFT_SPI_BAUDRATE (40 * 1000 * 1000)  // 40 MHz

// --------------------kích thước màn hình -------------------

#define TFT_WIDTH   240
#define TFT_HEIGHT  320

// --------------------các định nghĩa lệnh điều khiển ILI9341 -------------------

#define TFT_CMD_SWRESET   0x01
#define TFT_CMD_SLPOUT    0x11
#define TFT_CMD_DISPOFF   0x28
#define TFT_CMD_DISPON    0x29
#define TFT_CMD_CASET     0x2A
#define TFT_CMD_PASET     0x2B
#define TFT_CMD_RAMWR     0x2C
#define TFT_CMD_MADCTL    0x36
#define TFT_CMD_PIXFMT    0x3A

// --------------------các màu cơ bản (định dạng RGB565) -------------------

#define TFT_COLOR_BLACK   0x0000
#define TFT_COLOR_WHITE   0xFFFF
#define TFT_COLOR_RED     0xF800
#define TFT_COLOR_GREEN   0x07E0
#define TFT_COLOR_BLUE    0x001F
#define TFT_COLOR_YELLOW  0xFFE0
#define TFT_COLOR_CYAN    0x07FF

// --------------------các cấu trúc dữ liệu và hàm API-------------------

typedef enum {
    TFT_OK    = 0,
    TFT_ERROR = 1,
} TFT_Status;

// cấu trúc dữ liệu để lưu trữ thông tin về thiết bị màn hình
typedef struct {
    spi_inst_t *handle_spi;     // Con trỏ tới SPI instance (spi0 hoặc spi1)
    bool        is_initialized; // Trạng thái khởi tạo thành công hay chưa
} tft_dev_t;

// hàm khởi tạo màn hình (cấu hình GPIO, SPI, reset và gửi chuỗi lệnh khởi động ILI9341)
TFT_Status TFT_Init(tft_dev_t *dev, spi_inst_t *handle_spi);

// hàm tô toàn bộ màn hình bằng 1 màu duy nhất
TFT_Status TFT_FillScreen(tft_dev_t *dev, uint16_t color);

// hàm vẽ 1 ký tự tại toạ độ (x, y) sử dụng font 5x7, với màu chữ/nền và hệ số phóng to
TFT_Status TFT_DrawChar(tft_dev_t *dev, int16_t x, int16_t y, char c,
                         uint16_t fg_color, uint16_t bg_color, uint8_t size);

// hàm vẽ chuỗi ký tự tại toạ độ (x, y)
TFT_Status TFT_DrawString(tft_dev_t *dev, int16_t x, int16_t y, const char *str,
                           uint16_t fg_color, uint16_t bg_color, uint8_t size);

// hàm bật/tắt đèn nền màn hình
TFT_Status TFT_Backlight(tft_dev_t *dev, bool on);

#ifdef __cplusplus
}
#endif

#endif // TFT_H
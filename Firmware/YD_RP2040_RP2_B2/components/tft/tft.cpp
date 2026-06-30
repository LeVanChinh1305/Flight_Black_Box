#include "tft.h"
#include "components/font5x7/font5x7.h"
#include <stdio.h>
#include <string.h>

// ===================================================================
// Các hàm giao tiếp SPI
// ===================================================================

static inline void TFT_CS_Select(void)   { gpio_put(TFT_PIN_CS, 0); }
static inline void TFT_CS_Deselect(void) { gpio_put(TFT_PIN_CS, 1); }

static TFT_Status TFT_WriteCommand(tft_dev_t *dev, uint8_t cmd) {
    if (dev == NULL || dev->handle_spi == NULL) return TFT_ERROR;

    gpio_put(TFT_PIN_DC, 0); // Command mode
    TFT_CS_Select();
    spi_write_blocking(dev->handle_spi, &cmd, 1);
    TFT_CS_Deselect();

    return TFT_OK;
}

static TFT_Status TFT_WriteData(tft_dev_t *dev, const uint8_t *data, size_t len) {
    if (dev == NULL || dev->handle_spi == NULL) return TFT_ERROR;

    gpio_put(TFT_PIN_DC, 1); // Data mode
    TFT_CS_Select();
    spi_write_blocking(dev->handle_spi, data, len);
    TFT_CS_Deselect();

    return TFT_OK;
}

static TFT_Status TFT_WriteCommandData(tft_dev_t *dev, uint8_t cmd, const uint8_t *data, size_t len) {
    TFT_Status status = TFT_WriteCommand(dev, cmd);
    if (status != TFT_OK) return status;
    if (len > 0) return TFT_WriteData(dev, data, len);
    return TFT_OK;
}

// Đặt vùng vẽ
static TFT_Status TFT_SetWindow(tft_dev_t *dev, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t col[4] = { (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF),
                       (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF) };
    uint8_t row[4] = { (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF),
                       (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF) };

    TFT_WriteCommandData(dev, TFT_CMD_CASET, col, 4);
    TFT_WriteCommandData(dev, TFT_CMD_PASET, row, 4);
    return TFT_WriteCommand(dev, TFT_CMD_RAMWR);
}

// Push block - Đã tối ưu
static void TFT_PushBlock(tft_dev_t *dev, uint16_t color, int count) {
    uint8_t buf[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };

    gpio_put(TFT_PIN_DC, 1);
    TFT_CS_Select();

    while (count > 0) {
        int n = (count > 1024) ? 1024 : count;  // viết nhiều pixel cùng lúc
        for (int i = 0; i < n; i++) {
            spi_write_blocking(dev->handle_spi, buf, 2);
        }
        count -= n;
    }

    TFT_CS_Deselect();
}

// ===================================================================
// Khởi tạo màn hình
// ===================================================================

TFT_Status TFT_Init(tft_dev_t *dev, spi_inst_t *handle_spi) {
    if (dev == NULL || handle_spi == NULL) return TFT_ERROR;

    dev->handle_spi = handle_spi;
    dev->is_initialized = false;

    // 1. Khởi tạo SPI ban đầu với tốc độ thấp
    spi_init(dev->handle_spi, 2 * 1000 * 1000);
    
    // 2. Set chức năng chân SPI
    gpio_set_function(TFT_PIN_SDO, GPIO_FUNC_SPI); // MISO
    gpio_set_function(TFT_PIN_SDI, GPIO_FUNC_SPI); // MOSI
    gpio_set_function(TFT_PIN_SCL, GPIO_FUNC_SPI); // SCK

    // 3. Khởi tạo các chân điều khiển
    gpio_init(TFT_PIN_CS);  gpio_set_dir(TFT_PIN_CS,  GPIO_OUT);
    gpio_init(TFT_PIN_RST); gpio_set_dir(TFT_PIN_RST, GPIO_OUT);
    gpio_init(TFT_PIN_DC);  gpio_set_dir(TFT_PIN_DC,  GPIO_OUT);
    gpio_init(TFT_PIN_LED); gpio_set_dir(TFT_PIN_LED, GPIO_OUT);

    // 4. Trạng thái ban đầu
    gpio_put(TFT_PIN_CS, 1);
    gpio_put(TFT_PIN_RST, 1);
    gpio_put(TFT_PIN_DC, 1);
    gpio_put(TFT_PIN_LED, 0);

    // 5. Hardware Reset
    gpio_put(TFT_PIN_RST, 0);
    sleep_ms(50);
    gpio_put(TFT_PIN_RST, 1);
    sleep_ms(150);

    // Soft reset
    TFT_WriteCommand(dev, TFT_CMD_SWRESET);
    sleep_ms(150);

    // ================== Chuỗi init ILI9341 ==================
    TFT_WriteCommandData(dev, 0xEF, (uint8_t[]){0x03, 0x80, 0x02}, 3);
    TFT_WriteCommandData(dev, 0xCF, (uint8_t[]){0x00, 0xC1, 0x30}, 3);
    TFT_WriteCommandData(dev, 0xED, (uint8_t[]){0x64, 0x03, 0x12, 0x81}, 4);
    TFT_WriteCommandData(dev, 0xE8, (uint8_t[]){0x85, 0x00, 0x78}, 3);
    TFT_WriteCommandData(dev, 0xCB, (uint8_t[]){0x39, 0x2C, 0x00, 0x34, 0x02}, 5);
    TFT_WriteCommandData(dev, 0xF7, (uint8_t[]){0x20}, 1);
    TFT_WriteCommandData(dev, 0xEA, (uint8_t[]){0x00, 0x00}, 2);

    TFT_WriteCommandData(dev, 0xC0, (uint8_t[]){0x23}, 1);
    TFT_WriteCommandData(dev, 0xC1, (uint8_t[]){0x10}, 1);
    TFT_WriteCommandData(dev, 0xC5, (uint8_t[]){0x3E, 0x28}, 2);
    TFT_WriteCommandData(dev, 0xC7, (uint8_t[]){0x86}, 1);

    // MADCTL - Thử các giá trị khác nhau nếu cần
    uint8_t madctl = 0x40;        // ← Đã thay đổi từ 0x48
    TFT_WriteCommandData(dev, TFT_CMD_MADCTL, &madctl, 1);

    uint8_t pixfmt = 0x55;
    TFT_WriteCommandData(dev, TFT_CMD_PIXFMT, &pixfmt, 1);

    TFT_WriteCommandData(dev, 0xB1, (uint8_t[]){0x00, 0x18}, 2);
    TFT_WriteCommandData(dev, 0xB6, (uint8_t[]){0x08, 0x82, 0x27}, 3);

    TFT_WriteCommandData(dev, 0xF2, (uint8_t[]){0x00}, 1);
    TFT_WriteCommandData(dev, 0x26, (uint8_t[]){0x01}, 1);

    TFT_WriteCommandData(dev, 0xE0, (uint8_t[]){0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00}, 15);
    TFT_WriteCommandData(dev, 0xE1, (uint8_t[]){0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F}, 15);

    TFT_WriteCommand(dev, TFT_CMD_SLPOUT);
    sleep_ms(150);

    TFT_WriteCommand(dev, TFT_CMD_DISPON);
    sleep_ms(50);

    // Tăng tốc độ SPI sau khi init xong
    spi_set_baudrate(dev->handle_spi, TFT_SPI_BAUDRATE);  // 40MHz

    // Xóa màn hình
    TFT_FillScreen(dev, TFT_COLOR_BLACK);
    gpio_put(TFT_PIN_LED, 1);   // Bật backlight

    dev->is_initialized = true;
    printf("[TFT] Khoi tao hoan tat. MADCTL=0x%02X, SPI=40MHz\n", 0x40);
    return TFT_OK;
}

// ===================================================================
// Các hàm khác
// ===================================================================

TFT_Status TFT_Backlight(tft_dev_t *dev, bool on) {
    if (dev == NULL) return TFT_ERROR;
    gpio_put(TFT_PIN_LED, on ? 1 : 0);
    return TFT_OK;
}

TFT_Status TFT_FillScreen(tft_dev_t *dev, uint16_t color) {
    if (dev == NULL || dev->handle_spi == NULL) return TFT_ERROR;

    TFT_SetWindow(dev, 0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
    TFT_PushBlock(dev, color, TFT_WIDTH * TFT_HEIGHT);

    return TFT_OK;
}

TFT_Status TFT_DrawChar(tft_dev_t *dev, int16_t x, int16_t y, char c,
                         uint16_t fg_color, uint16_t bg_color, uint8_t size) {
    if (dev == NULL || dev->handle_spi == NULL) return TFT_ERROR;
    if (c < 32 || c > 126) c = '?';

    const uint8_t *glyph = font5x7[c - 32];

    for (int col = 0; col < 6; col++) {
        uint8_t line_bits = (col < 5) ? glyph[col] : 0x00;

        for (int row = 0; row < 8; row++) {
            uint16_t color = (line_bits & (1 << row)) ? fg_color : bg_color;

            int16_t px = x + col * size;
            int16_t py = y + row * size;

            if (px + size - 1 >= TFT_WIDTH || py + size - 1 >= TFT_HEIGHT) continue;

            TFT_SetWindow(dev, px, py, px + size - 1, py + size - 1);
            TFT_PushBlock(dev, color, size * size);
        }
    }
    return TFT_OK;
}

TFT_Status TFT_DrawString(tft_dev_t *dev, int16_t x, int16_t y, const char *str,
                           uint16_t fg_color, uint16_t bg_color, uint8_t size) {
    if (dev == NULL || dev->handle_spi == NULL || str == NULL) return TFT_ERROR;

    int16_t cursor_x = x;
    int16_t cursor_y = y;

    while (*str) {
        if (*str == '\n') {
            cursor_y += 8 * size + 2;
            cursor_x = x;
        } else {
            TFT_DrawChar(dev, cursor_x, cursor_y, *str, fg_color, bg_color, size);
            cursor_x += 6 * size;
        }
        str++;
    }
    return TFT_OK;
}
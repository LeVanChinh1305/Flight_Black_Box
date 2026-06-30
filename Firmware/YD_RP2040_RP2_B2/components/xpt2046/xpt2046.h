#ifndef XPT2046_H
#define XPT2046_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ================== Cấu hình chân SPI0 ==================
#define XPT2046_SPI_PORT     spi0
#define XPT2046_PIN_MISO     16     // T_DO  (SDO)
#define XPT2046_PIN_CS       17     // T_CS
#define XPT2046_PIN_SCK      18     // T_CLK
#define XPT2046_PIN_MOSI     19     // T_DIN (SDI)
#define XPT2046_PIN_IRQ      22     // T_IRQ (active-low)

// Kích thước màn hình
#define XPT2046_SCREEN_WIDTH   240
#define XPT2046_SCREEN_HEIGHT  320

// Độ phân giải ADC
#define XPT2046_MAX_VALUE    4095

// Số lần lấy mẫu để lọc nhiễu
#define XPT2046_SAMPLE_COUNT  5

typedef enum {
    XPT_OK    = 0,
    XPT_ERROR = 1,
} XPT_Status;

typedef struct {
    spi_inst_t *handle_spi;
    bool       is_initialized;
    bool       is_touched;
    uint16_t   x_raw;
    uint16_t   y_raw;
    int16_t    x;
    int16_t    y;
} xpt2046_dev_t;

// ====================== Khai báo hàm ======================
XPT_Status XPT2046_Init(xpt2046_dev_t *dev, spi_inst_t *handle_spi);
XPT_Status XPT2046_Update(xpt2046_dev_t *dev);
bool XPT2046_IsTouched(xpt2046_dev_t *dev);

// Hàm inline kiểm tra IRQ (dùng cho debug trong main)
static inline bool XPT_IsPressed(void) {
    return gpio_get(XPT2046_PIN_IRQ) == 0;
}

#ifdef __cplusplus
}
#endif

#endif // XPT2046_H
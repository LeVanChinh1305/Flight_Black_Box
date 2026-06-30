#include "xpt2046.h"
#include <stdio.h>

static inline void XPT_CS_Select(void)   { gpio_put(XPT2046_PIN_CS, 0); }
static inline void XPT_CS_Deselect(void) { gpio_put(XPT2046_PIN_CS, 1); }

static uint16_t XPT_ReadChannel(xpt2046_dev_t *dev, uint8_t cmd) {
    uint8_t tx[3] = {cmd, 0x00, 0x00};
    uint8_t rx[3] = {0};

    XPT_CS_Select();
    spi_write_read_blocking(dev->handle_spi, tx, rx, 3);
    XPT_CS_Deselect();

    return ((rx[1] << 8) | rx[2]) >> 3;
}

static uint16_t XPT_ReadChannelFiltered(xpt2046_dev_t *dev, uint8_t cmd) {
    uint16_t samples[XPT2046_SAMPLE_COUNT];
    for (int i = 0; i < XPT2046_SAMPLE_COUNT; i++) {
        samples[i] = XPT_ReadChannel(dev, cmd);
    }
    // Sắp xếp để lấy median
    for (int i = 0; i < XPT2046_SAMPLE_COUNT - 1; i++) {
        for (int j = i + 1; j < XPT2046_SAMPLE_COUNT; j++) {
            if (samples[j] < samples[i]) {
                uint16_t t = samples[i];
                samples[i] = samples[j];
                samples[j] = t;
            }
        }
    }
    return samples[XPT2046_SAMPLE_COUNT / 2];
}

XPT_Status XPT2046_Init(xpt2046_dev_t *dev, spi_inst_t *handle_spi) {
    if (dev == NULL || handle_spi == NULL) return XPT_ERROR;

    dev->handle_spi      = handle_spi;
    dev->is_initialized  = false;
    dev->is_touched      = false;
    dev->x = dev->y      = 0;
    dev->x_raw = dev->y_raw = 0;

    // ====================== Khởi tạo SPI0 ======================
    if (handle_spi == spi0) {
        spi_init(spi0, 2 * 1000 * 1000);  // 2MHz ban đầu

        gpio_set_function(XPT2046_PIN_SCK,  GPIO_FUNC_SPI);
        gpio_set_function(XPT2046_PIN_MOSI, GPIO_FUNC_SPI);
        gpio_set_function(XPT2046_PIN_MISO, GPIO_FUNC_SPI);
    }

    // CS
    gpio_init(XPT2046_PIN_CS);
    gpio_set_dir(XPT2046_PIN_CS, GPIO_OUT);
    gpio_put(XPT2046_PIN_CS, 1);

    // IRQ với pull-up
    gpio_init(XPT2046_PIN_IRQ);
    gpio_set_dir(XPT2046_PIN_IRQ, GPIO_IN);
    gpio_pull_up(XPT2046_PIN_IRQ);

    dev->is_initialized = true;
    printf("[XPT2046] Khoi tao xong. SPI0 | CS=GPIO%d, IRQ=GPIO%d\n",
           XPT2046_PIN_CS, XPT2046_PIN_IRQ);
    return XPT_OK;
}

XPT_Status XPT2046_Update(xpt2046_dev_t *dev) {
    if (dev == NULL || !dev->is_initialized) return XPT_ERROR;

    if (!XPT_IsPressed()) {
        dev->is_touched = false;
        return XPT_OK;
    }

    uint16_t x = XPT_ReadChannelFiltered(dev, 0xD0);
    uint16_t y = XPT_ReadChannelFiltered(dev, 0x90);

    // Kiểm tra lại sau khi đọc
    if (!XPT_IsPressed()) {
        dev->is_touched = false;
        return XPT_OK;
    }

    dev->x_raw = x;
    dev->y_raw = y;

    bool valid_range = (x > 150 && x < 3900 && y > 150 && y < 3900);

    if (valid_range) {
        dev->x = (int16_t)(x * XPT2046_SCREEN_WIDTH  / XPT2046_MAX_VALUE);
        dev->y = (int16_t)((XPT2046_MAX_VALUE - y) * XPT2046_SCREEN_HEIGHT / XPT2046_MAX_VALUE);
        dev->is_touched = true;

        printf("RAW: X=%4d Y=%4d | Scaled: (%3d, %3d)\n", x, y, dev->x, dev->y);
    } else {
        dev->is_touched = false;
    }

    return XPT_OK;
}

bool XPT2046_IsTouched(xpt2046_dev_t *dev) {
    return (dev && dev->is_touched);
}
#ifndef SDCARD_H
#define SDCARD_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// --------------------cấu hình chân SPI (DÙNG CHUNG BUS SPI0 với XPT2046) -------------------
// MISO/MOSI/SCK đã được cấu hình GPIO_FUNC_SPI bởi XPT2046_Init() vì cùng 1 bus vật lý.
// Module SD chỉ cần khởi tạo thêm chân CS riêng của nó.
#define SDCARD_SPI_PORT   spi0
#define SDCARD_PIN_MISO   16   // DO   - trùng XPT2046_PIN_MISO
#define SDCARD_PIN_MOSI   19   // DI   - trùng XPT2046_PIN_MOSI
#define SDCARD_PIN_SCK    18   // CLK  - trùng XPT2046_PIN_SCK
#define SDCARD_PIN_CS     23   // CS riêng cho thẻ nhớ (Output)

// --------------------tốc độ SPI -------------------
// Theo chuẩn SD-over-SPI: bắt buộc phải khởi tạo (gửi 80 xung clock + CMD0/CMD8/ACMD41) ở
// tốc độ THẤP (<= 400kHz), sau khi thẻ vào trạng thái sẵn sàng mới được tăng tốc độ lên.
#define SDCARD_BAUD_INIT    (400 * 1000)         // 400kHz lúc khởi tạo (bắt buộc theo chuẩn SD)
#define SDCARD_BAUD_NORMAL  (2 * 1000 * 1000)    // 2MHz lúc hoạt động bình thường
// Lưu ý: vì SPI0 dùng chung với XPT2046 (cũng đang chạy 2MHz cố định), driver SD KHÔNG
// chuyển bus lên tốc độ cao (>2MHz) để tránh ảnh hưởng tới việc đọc cảm ứng. Nếu sau này
// cần tăng tốc ghi log, phải tự thêm cơ chế đổi/khôi phục baudrate quanh từng giao dịch.

// --------------------lệnh (command) chuẩn SD-over-SPI -------------------
#define SDCARD_CMD0    0   // GO_IDLE_STATE
#define SDCARD_CMD8    8   // SEND_IF_COND
#define SDCARD_CMD9    9   // SEND_CSD
#define SDCARD_CMD10   10  // SEND_CID
#define SDCARD_CMD12   12  // STOP_TRANSMISSION
#define SDCARD_CMD16   16  // SET_BLOCKLEN
#define SDCARD_CMD17   17  // READ_SINGLE_BLOCK
#define SDCARD_CMD24   24  // WRITE_BLOCK
#define SDCARD_CMD55   55  // APP_CMD (báo lệnh kế tiếp là ACMD)
#define SDCARD_CMD58   58  // READ_OCR
#define SDCARD_ACMD41  41  // SD_SEND_OP_COND (gửi sau CMD55)

#define SDCARD_TOKEN_START_BLOCK   0xFE  // token bắt đầu 1 block dữ liệu (đọc/ghi đơn)
#define SDCARD_TOKEN_DATA_ACCEPTED 0x05  // token phản hồi sau khi ghi 1 block thành công

#define SDCARD_BLOCK_SIZE  512

// --------------------các cấu trúc dữ liệu và mã trạng thái-------------------

typedef enum {
    SD_OK      = 0,
    SD_ERROR   = 1,
    SD_TIMEOUT = 2,
} SD_Status;

typedef enum {
    SD_TYPE_UNKNOWN = 0,
    SD_TYPE_SDSC    = 1,   // Standard Capacity (≤2GB, địa chỉ theo BYTE)
    SD_TYPE_SDHC    = 2,   // High/Extended Capacity (địa chỉ theo BLOCK 512 byte)
} sd_card_type_t;

typedef struct {
    spi_inst_t     *handle_spi;
    sd_card_type_t  card_type;
    bool            is_initialized;
} sd_dev_t;

// --------------------API công khai-------------------

// Khởi tạo thẻ SD: cấu hình GPIO CS, thực hiện trình tự CMD0 -> CMD8 -> ACMD41 -> CMD58
// theo đúng chuẩn SD-over-SPI. Trả về SD_OK nếu thẻ vào trạng thái sẵn sàng (idle thoát).
SD_Status SDCARD_Init(sd_dev_t *dev, spi_inst_t *handle_spi);

// Kiểm tra nhanh thẻ còn phản hồi không (gửi CMD58 đọc OCR, không thay đổi trạng thái thẻ)
SD_Status SDCARD_CheckConnection(sd_dev_t *dev);

// Đọc 1 block 512 byte tại địa chỉ block_addr (đơn vị: số thứ tự block, không phải byte)
SD_Status SDCARD_ReadBlock(sd_dev_t *dev, uint32_t block_addr, uint8_t *buf);

// Ghi 1 block 512 byte tại địa chỉ block_addr
SD_Status SDCARD_WriteBlock(sd_dev_t *dev, uint32_t block_addr, const uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif // SDCARD_H
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

// --------------------cấu hình vùng "file log" giả lập ghi trực tiếp vào Raw Block -------------------
// Đây KHÔNG phải file thật theo nghĩa hệ điều hành (Windows sẽ không thấy nó), mà là 1 vùng
// block cố định trên thẻ được quản lý thủ công như 1 file ghi-tuần-tự (sequential log) đơn giản.
// Cấu trúc này hữu ích để test ghi/đọc/xoá mức phần cứng song song với FatFs.
#define SDLOG_HEADER_BLOCK      20000              // block chứa header (magic + số bản ghi)
#define SDLOG_DATA_START_BLOCK  (SDLOG_HEADER_BLOCK + 1) // block đầu tiên chứa dữ liệu
#define SDLOG_DATA_BLOCK_COUNT  50                 // 50 block = 25600 byte vùng chứa dữ liệu
#define SDLOG_RECORD_SIZE       32                 // mỗi bản ghi cố định 32 byte (chuỗi text, dư đệm '\0')
#define SDLOG_RECORDS_PER_BLOCK (SDCARD_BLOCK_SIZE / SDLOG_RECORD_SIZE) // 16 bản ghi / block
#define SDLOG_MAX_RECORDS       (SDLOG_DATA_BLOCK_COUNT * SDLOG_RECORDS_PER_BLOCK) // 800 bản ghi
#define SDLOG_MAGIC              0xABCD1234u

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

// --------------------API đọc dung lượng thẻ (qua thanh ghi CSD - CMD9)-------------------

// Đọc tổng dung lượng VẬT LÝ của thẻ (đơn vị byte), tính từ thanh ghi CSD.
// Đây là dung lượng THẬT của toàn bộ thẻ (không phụ thuộc filesystem).
SD_Status SDCARD_GetCapacityBytes(sd_dev_t *dev, uint64_t *total_bytes);

// --------------------API "file log" giả lập mức Raw Block (ghi tuần tự)-------------------
// Lưu ý: đây là vùng dữ liệu riêng do driver tự quản lý (không phải file .csv chuẩn hệ điều hành).
// Phù hợp để test ghi/đọc/xoá định kỳ; để quản lý file .csv thật mở được trên PC, hãy sử dụng thư viện FatFs (thư mục fatfs/).

// Tạo (hoặc reset) file log: ghi header với số bản ghi = 0.
SD_Status SDLOG_Create(sd_dev_t *dev);

// Thêm 1 bản ghi text (tối đa 31 ký tự + '\0') vào cuối file log.
// Trả về SD_ERROR nếu file đã đầy (SDLOG_MAX_RECORDS).
SD_Status SDLOG_Append(sd_dev_t *dev, const char *text);

// Đọc toàn bộ nội dung file log và in ra terminal (qua printf) theo thứ tự đã ghi.
SD_Status SDLOG_PrintAll(sd_dev_t *dev);

// Lấy số bản ghi hiện có trong file log (đọc từ header)
SD_Status SDLOG_GetRecordCount(sd_dev_t *dev, uint32_t *count);

// Xoá toàn bộ nội dung file log (reset số bản ghi về 0, tương đương "xoá file")
SD_Status SDLOG_Delete(sd_dev_t *dev);

#ifdef __cplusplus
}
#endif

#endif // SDCARD_H
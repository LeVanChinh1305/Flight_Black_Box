#include "sdcard.h"
#include <stdio.h>
#include <string.h>

// =====================================================================================
//  Driver thẻ nhớ SD - giao thức SPI mode (giống code mẫu chuẩn của hầu hết MCU 8/32-bit)
//  Chỉ hỗ trợ thẻ SDv2 (SDSC/SDHC) - hầu hết thẻ microSD bán hiện nay đều là SDv2 trở lên.
//  KHÔNG dùng FatFs - chỉ đọc/ghi block thô 512 byte để kiểm tra phần cứng.
// =====================================================================================

#define SD_INIT_RETRY        200    // số lần thử tối đa khi chờ ACMD41 thoát idle
#define SD_CMD_RESP_RETRY     16    // số lần thử tối đa khi chờ phản hồi lệnh (R1)
#define SD_READ_TOKEN_RETRY 4000    // số lần thử khi chờ token bắt đầu dữ liệu lúc đọc
#define SD_BUSY_WAIT_RETRY 100000   // số lần thử khi chờ thẻ hết bận sau khi ghi

// -------------------- các hàm giao tiếp SPI mức thấp --------------------

static inline void SD_CS_Select(void)   { gpio_put(SDCARD_PIN_CS, 0); }
static inline void SD_CS_Deselect(void) { gpio_put(SDCARD_PIN_CS, 1); }

// Trao đổi 1 byte qua SPI (gửi đồng thời nhận, đặc trưng giao thức SPI)
static uint8_t SD_Transfer(sd_dev_t *dev, uint8_t data) {
    uint8_t rx = 0xFF;
    spi_write_read_blocking(dev->handle_spi, &data, &rx, 1);
    return rx;
}

// Gửi N byte 0xFF chỉ để tạo xung clock (dùng lúc nhả CS hoặc chờ thẻ xử lý)
static void SD_SendDummyClocks(sd_dev_t *dev, int count) {
    for (int i = 0; i < count; i++) SD_Transfer(dev, 0xFF);
}

// Chờ thẻ hết bận: thẻ giữ MISO ở mức 0x00 khi đang bận ghi nội bộ, trả 0xFF khi sẵn sàng
static bool SD_WaitNotBusy(sd_dev_t *dev) {
    for (uint32_t i = 0; i < SD_BUSY_WAIT_RETRY; i++) {
        if (SD_Transfer(dev, 0xFF) == 0xFF) return true;
    }
    return false;
}

// Gửi 1 lệnh SD (6 byte: lệnh + 4 byte tham số + CRC) và chờ phản hồi R1 hợp lệ (bit7 = 0)
static uint8_t SD_SendCommand(sd_dev_t *dev, uint8_t cmd, uint32_t arg, uint8_t crc) {
    // CMD12 (STOP_TRANSMISSION) cần 1 byte rác trước đó theo chuẩn, các lệnh khác thì không bắt buộc
    SD_CS_Deselect();
    SD_Transfer(dev, 0xFF);
    SD_CS_Select();

    uint8_t frame[6];
    frame[0] = 0x40 | cmd;                 // 2 bit start (01) + 6 bit mã lệnh
    frame[1] = (uint8_t)(arg >> 24);
    frame[2] = (uint8_t)(arg >> 16);
    frame[3] = (uint8_t)(arg >> 8);
    frame[4] = (uint8_t)(arg);
    frame[5] = crc;                        // chỉ CMD0/CMD8 cần CRC đúng, còn lại thẻ thường bỏ qua khi không ở chế độ CRC bắt buộc

    for (int i = 0; i < 6; i++) SD_Transfer(dev, frame[i]);

    // Thẻ có thể trả về vài byte 0xFF (NCR) trước khi có phản hồi R1 thật sự
    uint8_t r1 = 0xFF;
    for (int i = 0; i < SD_CMD_RESP_RETRY; i++) {
        r1 = SD_Transfer(dev, 0xFF);
        if ((r1 & 0x80) == 0) break; // bit7 = 0 -> đã là phản hồi hợp lệ
    }
    return r1;
}

// -------------------- API công khai --------------------

SD_Status SDCARD_Init(sd_dev_t *dev, spi_inst_t *handle_spi) {
    if (dev == NULL || handle_spi == NULL) return SD_ERROR;

    dev->handle_spi    = handle_spi;
    dev->card_type      = SD_TYPE_UNKNOWN;
    dev->is_initialized = false;

    // Lưu ý: SPI0 (MISO/MOSI/SCK GPIO16/19/18) đã được spi_init() và gán GPIO_FUNC_SPI
    // bởi XPT2046_Init() vì dùng chung 1 bus vật lý. Ở đây chỉ cấu hình thêm chân CS riêng.
    gpio_init(SDCARD_PIN_CS);
    gpio_set_dir(SDCARD_PIN_CS, GPIO_OUT);
    gpio_put(SDCARD_PIN_CS, 1); // nhả CS trước, tránh xung đột với XPT2046 đang dùng chung bus

    // Hạ tốc độ SPI xuống mức an toàn cho lúc khởi tạo (bắt buộc theo chuẩn SD)
    spi_set_baudrate(handle_spi, SDCARD_BAUD_INIT);

    // Bước 1: gửi tối thiểu 74 xung clock với CS và MOSI ở mức cao để thẻ vào chế độ SPI
    SD_CS_Deselect();
    SD_SendDummyClocks(dev, 10); // 10 byte = 80 xung clock

    // Bước 2: CMD0 - GO_IDLE_STATE, đưa thẻ về trạng thái idle, chờ phản hồi R1 = 0x01
    uint8_t r1 = 0xFF;
    int retry = 0;
    do {
        r1 = SD_SendCommand(dev, SDCARD_CMD0, 0x00000000, 0x95); // CRC cố định đúng chuẩn cho CMD0
        retry++;
    } while (!(r1 & 0x01) && retry < SD_CMD_RESP_RETRY);

    SD_CS_Deselect();
    SD_Transfer(dev, 0xFF);

    if (!(r1 & 0x01)) {
        printf("[SDCARD] LOI: Khong vao duoc trang thai IDLE (CMD0). R1=0x%02X\n", r1);
        return SD_ERROR;
    }

    // Bước 3: CMD8 - phân biệt SDv2 (có CMD8) và SDv1 (không có CMD8)
    r1 = SD_SendCommand(dev, SDCARD_CMD8, 0x000001AA, 0x87);
    uint8_t r7[4] = {0};
    // SDv2 trả R1=0x01 (idle). SDv1 trả 0x05 (illegal cmd) hoặc 0x03 (idle+erase_reset).
    // Chỉ cần kiểm tra bit0=idle VÀ bit2=illegal_cmd=0 là đủ phân biệt.
    bool is_v2 = (r1 & 0x01) && !(r1 & 0x04);

    if (is_v2) {
        for (int i = 0; i < 4; i++) r7[i] = SD_Transfer(dev, 0xFF);
        SD_CS_Deselect();
        SD_Transfer(dev, 0xFF);
        if (!(r7[2] == 0x01 && r7[3] == 0xAA)) {
            printf("[SDCARD] LOI: The SDv2 nhung sai check pattern CMD8 (dien ap khong phu hop).\n");
            return SD_ERROR;
        }
        printf("[SDCARD] Phat hien the SDv2.\n");
    } else {
        // SDv1: drain byte rác (nếu có), nhả CS rồi tiếp tục bằng trình tự khởi tạo SDv1
        SD_CS_Deselect();
        SD_Transfer(dev, 0xFF);
        printf("[SDCARD] CMD8 = 0x%02X -> the SDv1, thu trinh tu SDv1...\n", r1);
    }

    // Bước 4: ACMD41 - SDv2 dùng arg 0x40000000 (bit HCS=1), SDv1 dùng arg 0x00000000
    uint32_t acmd41_arg = is_v2 ? 0x40000000 : 0x00000000;
    retry = 0;
    do {
        SD_SendCommand(dev, SDCARD_CMD55, 0x00000000, 0x01);
        r1 = SD_SendCommand(dev, SDCARD_ACMD41, acmd41_arg, 0x01);
        SD_CS_Deselect();
        SD_Transfer(dev, 0xFF);
        if (r1 == 0x00) break;
        sleep_ms(2);
        retry++;
    } while (retry < SD_INIT_RETRY);

    if (r1 != 0x00) {
        printf("[SDCARD] LOI: ACMD41 timeout (R1=0x%02X). Co the la the MMC (chua ho tro).\n", r1);
        return SD_TIMEOUT;
    }

    // Bước 5: CMD58 - đọc OCR, lấy bit CCS để phân biệt SDHC/SDXC và SDSC
    // SDv1 không có bit CCS -> tự động nhận là SDSC (đúng)
    r1 = SD_SendCommand(dev, SDCARD_CMD58, 0x00000000, 0x01);
    uint8_t ocr[4] = {0};
    if (r1 == 0x00) {
        for (int i = 0; i < 4; i++) ocr[i] = SD_Transfer(dev, 0xFF);
    }
    SD_CS_Deselect();
    SD_Transfer(dev, 0xFF);

    dev->card_type = (is_v2 && (ocr[0] & 0x40)) ? SD_TYPE_SDHC : SD_TYPE_SDSC;

    // Bước 6: ép block size = 512 cho SDSC (SDv1 và SDv2-SDSC đều cần, SDHC bỏ qua)
    if (dev->card_type == SD_TYPE_SDSC) {
        r1 = SD_SendCommand(dev, SDCARD_CMD16, SDCARD_BLOCK_SIZE, 0x01);
        SD_CS_Deselect();
        SD_Transfer(dev, 0xFF);
        if (r1 != 0x00) {
            printf("[SDCARD] LOI: Khong the ep block size 512 (CMD16). R1=0x%02X\n", r1);
            return SD_ERROR;
        }
    }

    // Khởi tạo xong -> nâng tốc độ SPI lên mức hoạt động bình thường
    spi_set_baudrate(handle_spi, SDCARD_BAUD_NORMAL);

    dev->is_initialized = true;
    printf("[SDCARD] Khoi tao OK. Loai the: %s | CS=GPIO%d (chung bus SPI0 voi XPT2046)\n",
           (dev->card_type == SD_TYPE_SDHC) ? "SDHC/SDXC" : "SDSC",
           SDCARD_PIN_CS);
    return SD_OK;
}

SD_Status SDCARD_CheckConnection(sd_dev_t *dev) {
    if (dev == NULL || !dev->is_initialized) return SD_ERROR;

    uint8_t r1 = SD_SendCommand(dev, SDCARD_CMD58, 0x00000000, 0x01);
    if (r1 == 0x00) {
        SD_SendDummyClocks(dev, 4); // đọc bỏ 4 byte OCR trả về để giải phóng bus đúng cách
    }
    SD_CS_Deselect();
    SD_Transfer(dev, 0xFF);

    return (r1 == 0x00) ? SD_OK : SD_ERROR;
}

SD_Status SDCARD_ReadBlock(sd_dev_t *dev, uint32_t block_addr, uint8_t *buf) {
    if (dev == NULL || !dev->is_initialized || buf == NULL) return SD_ERROR;

    // SDHC/SDXC: địa chỉ tính theo SỐ BLOCK. SDSC: địa chỉ tính theo BYTE -> phải nhân 512.
    uint32_t arg = (dev->card_type == SD_TYPE_SDHC) ? block_addr : (block_addr * SDCARD_BLOCK_SIZE);

    uint8_t r1 = SD_SendCommand(dev, SDCARD_CMD17, arg, 0x01);
    if (r1 != 0x00) {
        SD_CS_Deselect();
        SD_Transfer(dev, 0xFF);
        printf("[SDCARD] LOI: CMD17 (doc block) that bai. R1=0x%02X\n", r1);
        return SD_ERROR;
    }

    // Chờ token bắt đầu dữ liệu (0xFE)
    uint8_t token = 0xFF;
    uint32_t retry = 0;
    while (token != SDCARD_TOKEN_START_BLOCK && retry < SD_READ_TOKEN_RETRY) {
        token = SD_Transfer(dev, 0xFF);
        retry++;
    }
    if (token != SDCARD_TOKEN_START_BLOCK) {
        SD_CS_Deselect();
        SD_Transfer(dev, 0xFF);
        printf("[SDCARD] LOI: Khong nhan duoc token bat dau du lieu khi doc.\n");
        return SD_TIMEOUT;
    }

    // Đọc 512 byte dữ liệu
    for (int i = 0; i < SDCARD_BLOCK_SIZE; i++) buf[i] = SD_Transfer(dev, 0xFF);

    // Đọc bỏ 2 byte CRC theo sau (driver này không kiểm tra CRC)
    SD_Transfer(dev, 0xFF);
    SD_Transfer(dev, 0xFF);

    SD_CS_Deselect();
    SD_Transfer(dev, 0xFF);
    return SD_OK;
}

SD_Status SDCARD_WriteBlock(sd_dev_t *dev, uint32_t block_addr, const uint8_t *buf) {
    if (dev == NULL || !dev->is_initialized || buf == NULL) return SD_ERROR;

    uint32_t arg = (dev->card_type == SD_TYPE_SDHC) ? block_addr : (block_addr * SDCARD_BLOCK_SIZE);

    uint8_t r1 = SD_SendCommand(dev, SDCARD_CMD24, arg, 0x01);
    if (r1 != 0x00) {
        SD_CS_Deselect();
        SD_Transfer(dev, 0xFF);
        printf("[SDCARD] LOI: CMD24 (ghi block) that bai. R1=0x%02X\n", r1);
        return SD_ERROR;
    }

    // Gửi 1 byte đệm rồi tới token bắt đầu dữ liệu
    SD_Transfer(dev, 0xFF);
    SD_Transfer(dev, SDCARD_TOKEN_START_BLOCK);

    // Ghi 512 byte dữ liệu
    for (int i = 0; i < SDCARD_BLOCK_SIZE; i++) SD_Transfer(dev, buf[i]);

    // Gửi 2 byte CRC giả (không dùng chế độ kiểm tra CRC nên giá trị không quan trọng)
    SD_Transfer(dev, 0xFF);
    SD_Transfer(dev, 0xFF);

    // Đọc token phản hồi từ thẻ, chỉ 5 bit thấp có ý nghĩa: 0b101 (0x05) = đã chấp nhận dữ liệu
    uint8_t data_resp = SD_Transfer(dev, 0xFF);
    if ((data_resp & 0x1F) != SDCARD_TOKEN_DATA_ACCEPTED) {
        SD_CS_Deselect();
        SD_Transfer(dev, 0xFF);
        printf("[SDCARD] LOI: The tu choi du lieu khi ghi. Token=0x%02X\n", data_resp);
        return SD_ERROR;
    }

    // Chờ thẻ ghi xong nội bộ (busy) trước khi nhả CS
    if (!SD_WaitNotBusy(dev)) {
        SD_CS_Deselect();
        SD_Transfer(dev, 0xFF);
        printf("[SDCARD] LOI: Timeout cho the het ban sau khi ghi.\n");
        return SD_TIMEOUT;
    }

    SD_CS_Deselect();
    SD_Transfer(dev, 0xFF);
    return SD_OK;
}

// =====================================================================================
//  ĐỌC DUNG LƯỢNG THẺ (CMD9 - SEND_CSD)
// =====================================================================================

// Đọc 1 khối dữ liệu độ dài tuỳ ý (dùng chung cho CMD9-CSD/CMD10-CID, không chỉ 512 byte)
// Giả định CS đã được Select trước đó và lệnh đã gửi xong, hàm này chỉ lo phần nhận data token.
static SD_Status SD_ReceiveDataBlock(sd_dev_t *dev, uint8_t *buf, uint16_t len) {
    uint8_t token = 0xFF;
    uint32_t retry = 0;
    while (token != SDCARD_TOKEN_START_BLOCK && retry < SD_READ_TOKEN_RETRY) {
        token = SD_Transfer(dev, 0xFF);
        retry++;
    }
    if (token != SDCARD_TOKEN_START_BLOCK) return SD_TIMEOUT;

    for (uint16_t i = 0; i < len; i++) buf[i] = SD_Transfer(dev, 0xFF);
    SD_Transfer(dev, 0xFF); // CRC byte cao
    SD_Transfer(dev, 0xFF); // CRC byte thấp
    return SD_OK;
}

SD_Status SDCARD_GetCapacityBytes(sd_dev_t *dev, uint64_t *total_bytes) {
    if (dev == NULL || !dev->is_initialized || total_bytes == NULL) return SD_ERROR;

    uint8_t r1 = SD_SendCommand(dev, SDCARD_CMD9, 0x00000000, 0x01);
    if (r1 != 0x00) {
        SD_CS_Deselect();
        SD_Transfer(dev, 0xFF);
        printf("[SDCARD] LOI: CMD9 (doc CSD) that bai. R1=0x%02X\n", r1);
        return SD_ERROR;
    }

    uint8_t csd[16] = {0};
    SD_Status st = SD_ReceiveDataBlock(dev, csd, 16);
    SD_CS_Deselect();
    SD_Transfer(dev, 0xFF);

    if (st != SD_OK) {
        printf("[SDCARD] LOI: Khong nhan duoc thanh ghi CSD (timeout).\n");
        return st;
    }

    uint8_t csd_version = csd[0] >> 6; // bit [127:126]: 0 = CSD v1.0 (SDSC), 1 = CSD v2.0 (SDHC/SDXC)

    if (csd_version == 1) {
        // CSD v2.0 (SDHC/SDXC): C_SIZE là số nguyên 22 bit tại CSD[7][5:0]..CSD[9]
        uint32_t c_size = ((uint32_t)(csd[7] & 0x3F) << 16) | ((uint32_t)csd[8] << 8) | csd[9];
        *total_bytes = (uint64_t)(c_size + 1) * 512ULL * 1024ULL; // dung lượng = (C_SIZE+1) * 512KB
    } else {
        // CSD v1.0 (SDSC): công thức kinh điển dùng C_SIZE (12 bit) + C_SIZE_MULT (3 bit) + READ_BL_LEN (4 bit)
        uint32_t c_size      = ((uint32_t)(csd[6] & 0x03) << 10) | ((uint32_t)csd[7] << 2) | (csd[8] >> 6);
        uint32_t c_size_mult = ((uint32_t)(csd[9] & 0x03) << 1) | (csd[10] >> 7);
        uint32_t read_bl_len = csd[5] & 0x0F;
        *total_bytes = (uint64_t)(c_size + 1) * (1ULL << (c_size_mult + 2)) * (1ULL << read_bl_len);
    }

    return SD_OK;
}

// =====================================================================================
//  "FILE LOG" GIẢ LẬP - GHI TUẦN TỰ TRÊN VÙNG BLOCK RIÊNG (KHÔNG CẦN FAT32)
//  Cấu trúc:
//   - Block header (SDLOG_HEADER_BLOCK): 4 byte magic + 4 byte record_count, còn lại bỏ trống.
//   - Block dữ liệu (từ SDLOG_DATA_START_BLOCK): mỗi block chứa 16 bản ghi x 32 byte.
//  Việc thêm 1 bản ghi luôn phải đọc nguyên block chứa nó, ghi đè đúng 32 byte rồi ghi lại
//  cả block (vì SD chỉ hỗ trợ đọc/ghi theo đơn vị block 512 byte, không ghi được từng byte lẻ).
// =====================================================================================

static SD_Status SDLOG_ReadHeader(sd_dev_t *dev, uint32_t *magic_out, uint32_t *count_out) {
    uint8_t hdr[SDCARD_BLOCK_SIZE];
    SD_Status st = SDCARD_ReadBlock(dev, SDLOG_HEADER_BLOCK, hdr);
    if (st != SD_OK) return st;

    memcpy(magic_out, &hdr[0], sizeof(uint32_t));
    memcpy(count_out, &hdr[4], sizeof(uint32_t));
    return SD_OK;
}

static SD_Status SDLOG_WriteHeader(sd_dev_t *dev, uint32_t magic, uint32_t count) {
    uint8_t hdr[SDCARD_BLOCK_SIZE];
    memset(hdr, 0, SDCARD_BLOCK_SIZE);
    memcpy(&hdr[0], &magic, sizeof(uint32_t));
    memcpy(&hdr[4], &count, sizeof(uint32_t));
    return SDCARD_WriteBlock(dev, SDLOG_HEADER_BLOCK, hdr);
}

SD_Status SDLOG_Create(sd_dev_t *dev) {
    if (dev == NULL || !dev->is_initialized) return SD_ERROR;

    SD_Status st = SDLOG_WriteHeader(dev, SDLOG_MAGIC, 0);
    if (st == SD_OK) {
        printf("[SDLOG] Da tao (reset) file log tai block %u, vung du lieu %u block (%u byte).\n",
               (unsigned)SDLOG_HEADER_BLOCK, (unsigned)SDLOG_DATA_BLOCK_COUNT,
               (unsigned)(SDLOG_DATA_BLOCK_COUNT * SDCARD_BLOCK_SIZE));
    } else {
        printf("[SDLOG] LOI: Khong the tao file log (ghi header that bai).\n");
    }
    return st;
}

SD_Status SDLOG_Append(sd_dev_t *dev, const char *text) {
    if (dev == NULL || !dev->is_initialized || text == NULL) return SD_ERROR;

    uint32_t magic = 0, count = 0;
    if (SDLOG_ReadHeader(dev, &magic, &count) != SD_OK) return SD_ERROR;

    if (magic != SDLOG_MAGIC) {
        printf("[SDLOG] LOI: Chua co file log hop le (hay goi SDLOG_Create() truoc).\n");
        return SD_ERROR;
    }
    if (count >= SDLOG_MAX_RECORDS) {
        printf("[SDLOG] LOI: File log da day (%u/%u ban ghi).\n", (unsigned)count, (unsigned)SDLOG_MAX_RECORDS);
        return SD_ERROR;
    }

    uint32_t block_index   = SDLOG_DATA_START_BLOCK + (count / SDLOG_RECORDS_PER_BLOCK);
    uint32_t offset_inside = (count % SDLOG_RECORDS_PER_BLOCK) * SDLOG_RECORD_SIZE;

    // Đọc nguyên block hiện tại (vì các bản ghi khác trong cùng block phải được giữ nguyên)
    uint8_t block_buf[SDCARD_BLOCK_SIZE];
    if (offset_inside == 0) {
        // Bản ghi đầu tiên của block này -> chưa có dữ liệu cũ cần giữ, khởi tạo block trắng cho nhanh
        memset(block_buf, 0, SDCARD_BLOCK_SIZE);
    } else {
        if (SDCARD_ReadBlock(dev, block_index, block_buf) != SD_OK) return SD_ERROR;
    }

    // Ghi đè đúng 32 byte của bản ghi mới (cắt bớt nếu chuỗi quá dài, đảm bảo có '\0' kết thúc)
    char record[SDLOG_RECORD_SIZE];
    memset(record, 0, SDLOG_RECORD_SIZE);
    strncpy(record, text, SDLOG_RECORD_SIZE - 1);
    memcpy(&block_buf[offset_inside], record, SDLOG_RECORD_SIZE);

    if (SDCARD_WriteBlock(dev, block_index, block_buf) != SD_OK) return SD_ERROR;

    // Cập nhật header (persist ngay để không mất dữ liệu nếu mất điện đột ngột)
    return SDLOG_WriteHeader(dev, SDLOG_MAGIC, count + 1);
}

SD_Status SDLOG_GetRecordCount(sd_dev_t *dev, uint32_t *count) {
    if (dev == NULL || !dev->is_initialized || count == NULL) return SD_ERROR;
    uint32_t magic = 0;
    SD_Status st = SDLOG_ReadHeader(dev, &magic, count);
    if (st == SD_OK && magic != SDLOG_MAGIC) {
        *count = 0;
        return SD_ERROR; // chưa từng SDLOG_Create()
    }
    return st;
}

SD_Status SDLOG_PrintAll(sd_dev_t *dev) {
    if (dev == NULL || !dev->is_initialized) return SD_ERROR;

    uint32_t magic = 0, count = 0;
    if (SDLOG_ReadHeader(dev, &magic, &count) != SD_OK) return SD_ERROR;
    if (magic != SDLOG_MAGIC) {
        printf("[SDLOG] Chua co file log hop le.\n");
        return SD_ERROR;
    }

    printf("[SDLOG] ---- Noi dung file log (%u ban ghi) ----\n", (unsigned)count);
    uint8_t block_buf[SDCARD_BLOCK_SIZE];
    uint32_t last_block = 0xFFFFFFFF;

    for (uint32_t i = 0; i < count; i++) {
        uint32_t block_index   = SDLOG_DATA_START_BLOCK + (i / SDLOG_RECORDS_PER_BLOCK);
        uint32_t offset_inside = (i % SDLOG_RECORDS_PER_BLOCK) * SDLOG_RECORD_SIZE;

        if (block_index != last_block) {
            if (SDCARD_ReadBlock(dev, block_index, block_buf) != SD_OK) {
                printf("[SDLOG] LOI: Khong doc duoc block %u.\n", (unsigned)block_index);
                return SD_ERROR;
            }
            last_block = block_index;
        }

        char record[SDLOG_RECORD_SIZE + 1];
        memcpy(record, &block_buf[offset_inside], SDLOG_RECORD_SIZE);
        record[SDLOG_RECORD_SIZE] = '\0'; // đảm bảo kết thúc chuỗi dù dữ liệu gốc không có '\0'
        printf("  [%4u] %s\n", (unsigned)i, record);
    }
    printf("[SDLOG] ---- Het noi dung ----\n");
    return SD_OK;
}

SD_Status SDLOG_Delete(sd_dev_t *dev) {
    if (dev == NULL || !dev->is_initialized) return SD_ERROR;

    // "Xoá file" ở đây = đưa số bản ghi về 0 (giống quick-format), dữ liệu vật lý cũ vẫn còn
    // trên thẻ nhưng bị coi là không hợp lệ vì nằm ngoài record_count -> lần ghi sau sẽ đè lên.
    SD_Status st = SDLOG_WriteHeader(dev, SDLOG_MAGIC, 0);
    if (st == SD_OK) {
        printf("[SDLOG] Da xoa file log (reset ve 0 ban ghi).\n");
    } else {
        printf("[SDLOG] LOI: Xoa file log that bai.\n");
    }
    return st;
}
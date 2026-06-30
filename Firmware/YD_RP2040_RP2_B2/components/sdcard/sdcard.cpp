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
    } while (r1 != 0x01 && retry < SD_CMD_RESP_RETRY);

    SD_CS_Deselect();
    SD_Transfer(dev, 0xFF);

    if (r1 != 0x01) {
        printf("[SDCARD] LOI: Khong vao duoc trang thai IDLE (CMD0). R1=0x%02X\n", r1);
        return SD_ERROR;
    }

    // Bước 3: CMD8 - SEND_IF_COND, kiểm tra thẻ có hỗ trợ chuẩn SDv2 và điện áp 3.3V không
    // Tham số 0x1AA = VHS(0x1=2.7-3.6V) + check pattern (0xAA)
    r1 = SD_SendCommand(dev, SDCARD_CMD8, 0x000001AA, 0x87);
    uint8_t r7[4] = {0};
    bool is_v2 = (r1 == 0x01);
    if (is_v2) {
        for (int i = 0; i < 4; i++) r7[i] = SD_Transfer(dev, 0xFF);
    }
    SD_CS_Deselect();
    SD_Transfer(dev, 0xFF);

    if (is_v2 && !(r7[2] == 0x01 && r7[3] == 0xAA)) {
        printf("[SDCARD] LOI: The khong ho tro dien ap 3.3V hoac sai check pattern (CMD8).\n");
        return SD_ERROR;
    }
    if (!is_v2) {
        // Không phản hồi CMD8 -> thẻ SDv1 hoặc MMC, driver tối giản này chưa hỗ trợ
        printf("[SDCARD] LOI: The khong phan hoi CMD8 (co the la the SDv1/MMC cu, chua duoc ho tro).\n");
        return SD_ERROR;
    }

    // Bước 4: gửi CMD55 + ACMD41 lặp lại cho đến khi thẻ thoát trạng thái idle (R1 = 0x00)
    retry = 0;
    do {
        SD_SendCommand(dev, SDCARD_CMD55, 0x00000000, 0x01); // báo lệnh kế tiếp là ACMD
        r1 = SD_SendCommand(dev, SDCARD_ACMD41, 0x40000000, 0x01); // bit HCS=1: cho phép thẻ trả lời là SDHC
        SD_CS_Deselect();
        SD_Transfer(dev, 0xFF);
        if (r1 == 0x00) break;
        sleep_ms(2);
        retry++;
    } while (retry < SD_INIT_RETRY);

    if (r1 != 0x00) {
        printf("[SDCARD] LOI: The khong thoat duoc trang thai IDLE sau ACMD41 (timeout). R1=0x%02X\n", r1);
        return SD_TIMEOUT;
    }

    // Bước 5: CMD58 - đọc thanh ghi OCR để xác định thẻ là SDHC/SDXC hay SDSC
    r1 = SD_SendCommand(dev, SDCARD_CMD58, 0x00000000, 0x01);
    uint8_t ocr[4] = {0};
    if (r1 == 0x00) {
        for (int i = 0; i < 4; i++) ocr[i] = SD_Transfer(dev, 0xFF);
    }
    SD_CS_Deselect();
    SD_Transfer(dev, 0xFF);

    dev->card_type = (ocr[0] & 0x40) ? SD_TYPE_SDHC : SD_TYPE_SDSC; // bit CCS nằm ở bit6 byte đầu OCR

    // Bước 6: chỉ với thẻ SDSC mới cần ép độ dài block = 512 byte (SDHC luôn cố định 512)
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
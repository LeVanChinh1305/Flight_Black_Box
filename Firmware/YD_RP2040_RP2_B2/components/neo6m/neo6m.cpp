#include "neo6m.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

// Kiem tra field co du do dai toi thieu truoc khi doc theo chi so
// (tranh doc phai rac stack khi ban tin bi cat ngan / thieu ky tu)
static bool field_len_ok(const char *field, size_t min_len) {
    return field != NULL && strlen(field) >= min_len;
}

// Chuyển đổi tọa độ định dạng NMEA (DDDMM.MMMM) sang độ thập phân (Decimal Degree)
static float nmea_to_decimal(const char *raw, char direction) {
    if (raw == NULL || raw[0] == '\0') return 0.0f;

    float raw_val = atof(raw);
    int   degrees = (int)(raw_val / 100);          
    float minutes = raw_val - (degrees * 100.0f);  
    float decimal = degrees + (minutes / 60.0f);

    if (direction == 'S' || direction == 'W') decimal = -decimal;

    return decimal;
}

// Hàm bổ trợ tách chuỗi con theo dấu phẩy NMEA (Tối ưu hóa: O(N))
static void get_field(const char *sentence, int index, char *out, uint8_t out_size) {
    out[0] = '\0';
    if (!sentence) return;

    int current_field = 0;
    const char *p = sentence;

    // Duyệt qua các trường cho đến khi tới field cần lấy
    while (current_field < index && *p != '\0') {
        if (*p == ',') {
            current_field++;
        }
        p++;
    }

    if (current_field == index) {
        int i = 0;
        // Copy giá trị của trường vào out
        while (*p != ',' && *p != '*' && *p != '\0' && i < (out_size - 1)) {
            out[i++] = *p++;
        }
        out[i] = '\0';
    }
}

// Kiểm tra mã Checksum cuối câu NMEA
bool NEO6M_ValidateChecksum(const char *sentence) {
    if (sentence == NULL || sentence[0] != '$') return false;

    uint8_t     calc = 0;
    const char *p    = sentence + 1;

    while (*p != '*' && *p != '\0') {
        calc ^= (uint8_t)(*p);
        p++;
    }
    if (*p != '*') return false;

    uint8_t recv = (uint8_t)strtol(p + 1, NULL, 16);
    return (calc == recv);
}

// ====================== HÀM GỬI LỆNH UBX ======================
static void NEO6M_SendUBX(neo6m_dev_t *dev, const uint8_t *msg, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        uart_putc(dev->handle_uart, msg[i]);
    }
    // Đợi module xử lý
    vTaskDelay(pdMS_TO_TICKS(50));
}

// Cấu hình chỉ bật GGA và RMC, tắt các câu còn lại
static void NEO6M_ConfigureNMEA(neo6m_dev_t *dev) {
    // UBX-CFG-MSG: Disable GLL
    uint8_t disableGLL[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x2B};
    // Disable GSA
    uint8_t disableGSA[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x32};
    // Disable GSV
    uint8_t disableGSV[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x39};
    // Disable VTG
    uint8_t disableVTG[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x47};

    // Enable GGA (rate = 1)
    uint8_t enableGGA[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x05, 0x38};
    // Enable RMC (rate = 1)
    uint8_t enableRMC[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x09, 0x54};

    printf("[NEO6M] Dang cau hinh chi xuat GGA + RMC...\n");

    NEO6M_SendUBX(dev, disableGLL, sizeof(disableGLL));
    NEO6M_SendUBX(dev, disableGSA, sizeof(disableGSA));
    NEO6M_SendUBX(dev, disableGSV, sizeof(disableGSV));
    NEO6M_SendUBX(dev, disableVTG, sizeof(disableVTG));

    NEO6M_SendUBX(dev, enableGGA, sizeof(enableGGA));
    NEO6M_SendUBX(dev, enableRMC, sizeof(enableRMC));

    // Lưu cấu hình vào flash (nếu module hỗ trợ)
    uint8_t saveConfig[] = {0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x31};
    NEO6M_SendUBX(dev, saveConfig, sizeof(saveConfig));

    printf("[NEO6M] Da cau hinh NMEA: chi con GGA va RMC\n");
}

// UBX-CFG-PRT (Class 0x06, ID 0x00): doi baudrate cua UART1 tren module.
// Payload co dinh: portID=1(UART1), mode=8N1(0x000008D0), inProto=UBX+NMEA+RTCM(0x0007),
// outProto=UBX+NMEA(0x0003). Checksum (2 byte cuoi) da duoc tinh san theo chuan UBX
// (Fletcher-8) cho tung gia tri baudrate tuong ung - KHONG sua payload ma khong tinh lai checksum.
static bool NEO6M_ChangeBaudrate(neo6m_dev_t *dev, uint32_t new_baud) {
    uint8_t setBaud38400[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00,
                               0xD0, 0x08, 0x00, 0x00, 0x00, 0x96, 0x00, 0x00, 0x07, 0x00,
                               0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x93, 0x90};
    uint8_t setBaud57600[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00,
                               0xD0, 0x08, 0x00, 0x00, 0x00, 0xE1, 0x00, 0x00, 0x07, 0x00,
                               0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDE, 0xC9};

    uint8_t *pkt = NULL;
    uint16_t pkt_len = 0;

    switch (new_baud) {
        case 38400: pkt = setBaud38400; pkt_len = sizeof(setBaud38400); break;
        case 57600: pkt = setBaud57600; pkt_len = sizeof(setBaud57600); break;
        default:
            printf("[NEO6M] Baud %lu chua co goi UBX dung san, bo qua doi baud\n", (unsigned long)new_baud);
            return false;
    }

    printf("[NEO6M] Dang doi baudrate module sang %lu...\n", (unsigned long)new_baud);
    NEO6M_SendUBX(dev, pkt, pkt_len);

    // Module can chut thoi gian ap dung cau hinh UART moi truoc khi MCU doi baud theo
    vTaskDelay(pdMS_TO_TICKS(100));
    uart_set_baudrate(dev->handle_uart, new_baud);
    dev->baudrate = new_baud;

    // Cho tin hieu on dinh o baud moi
    vTaskDelay(pdMS_TO_TICKS(100));
    printf("[NEO6M] Da chuyen UART sang %lu baud\n", (unsigned long)new_baud);
    return true;
}

// Khởi tạo phần cứng UART cho module GPS
NEO6M_Status NEO6M_Init(neo6m_dev_t *dev, uart_inst_t *handle_uart, uint pin_tx, uint pin_rx, uint32_t baudrate) {
    if (dev == NULL || handle_uart == NULL) return NEO6M_ERROR;

    dev->handle_uart    = handle_uart;
    dev->pin_tx         = pin_tx;
    dev->pin_rx         = pin_rx;
    dev->baudrate       = baudrate;
    dev->is_initialized = false;
    dev->buf_idx        = 0;
    memset(dev->nmea_buf, 0, NEO6M_NMEA_MAX_LEN);

    uart_init(dev->handle_uart, dev->baudrate);
    uart_set_format(dev->handle_uart, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(dev->handle_uart, true);

    gpio_set_function(dev->pin_tx, GPIO_FUNC_UART);
    gpio_set_function(dev->pin_rx, GPIO_FUNC_UART);

    // Dung vTaskDelay thay vi sleep_ms de nhuong CPU cho scheduler FreeRTOS
    // (sleep_ms cua pico-sdk la busy/blocking, khong "RTOS-aware")
    vTaskDelay(pdMS_TO_TICKS(500));

    dev->is_initialized = true;
    NEO6M_ConfigureNMEA(dev);

    // Tang baud sau khi da cau hinh xong NMEA, giup giam thoi gian truyen moi cau
    // (xem giai thich o NEO6M_TARGET_BAUDRATE trong neo6m.h). Neu that bai (baud
    // khong ho tro), module + MCU van tiep tuc chay o dev->baudrate hien tai (khong loi).
    #if (NEO6M_TARGET_BAUDRATE != NEO6M_BAUDRATE)
    NEO6M_ChangeBaudrate(dev, NEO6M_TARGET_BAUDRATE);
    #endif

    printf("[NEO6M] Khoi tao UART%d OK - Baud: %lu\n", uart_get_index(dev->handle_uart), (unsigned long)dev->baudrate);

    return NEO6M_OK;
}

// Đọc dòng ký tự liên tục từ UART cho đến khi gặp ký tự xuống dòng '\n'
NEO6M_Status NEO6M_ReadLine(neo6m_dev_t *dev) {
    if (dev == NULL || dev->handle_uart == NULL) return NEO6M_ERROR;

    dev->buf_idx = 0;
    memset(dev->nmea_buf, 0, NEO6M_NMEA_MAX_LEN);

    absolute_time_t deadline = make_timeout_time_ms(NEO6M_LINE_SEARCH_TIMEOUT_MS); // Tim ky tu bat dau '$'

    bool started = false;

    while (!time_reached(deadline)) {
        if (uart_is_readable(dev->handle_uart)) {
            char c = uart_getc(dev->handle_uart);

            if (!started) {
                if (c == '$') {
                    dev->nmea_buf[0] = c;
                    dev->buf_idx = 1;
                    started = true;
                    deadline = make_timeout_time_ms(NEO6M_LINE_READ_TIMEOUT_MS); // reset timeout khi bắt đầu dòng
                }
                continue;
            }

            if (dev->buf_idx < (NEO6M_NMEA_MAX_LEN - 1)) {
                dev->nmea_buf[dev->buf_idx++] = c;
            }

            if (c == '\n') {
                dev->nmea_buf[dev->buf_idx] = '\0';
                return NEO6M_OK;
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(1)); // Nhường CPU mạnh hơn
        }
    }

    return NEO6M_ERROR; // Timeout
}

// Xử lý gói tin mẫu $GPGGA (Vị trí, Cao độ, Số lượng vệ tinh)
NEO6M_Status NEO6M_ParseGGA(const char *sentence, neo6m_data_t *data) {
    if (sentence == NULL || data == NULL) return NEO6M_ERROR;
    if (strncmp(sentence, NEO6M_NMEA_GGA, 6) != 0) return NEO6M_ERROR;
    if (!NEO6M_ValidateChecksum(sentence)) return NEO6M_ERROR;

    char field[20];

    // Thời gian UTC
    get_field(sentence, 1, field, sizeof(field));
    if (field_len_ok(field, 6)) {
        char tmp[3] = {0};
        tmp[0] = field[0]; tmp[1] = field[1]; data->hour   = (uint8_t)atoi(tmp);
        tmp[0] = field[2]; tmp[1] = field[3]; data->minute = (uint8_t)atoi(tmp);
        tmp[0] = field[4]; tmp[1] = field[5]; data->second = (uint8_t)atoi(tmp);
    }

    // Vĩ độ
    char lat_raw[15] = {0}, lat_dir[2] = {0};
    get_field(sentence, 2, lat_raw, sizeof(lat_raw));
    get_field(sentence, 3, lat_dir, sizeof(lat_dir));
    data->latitude = nmea_to_decimal(lat_raw, lat_dir[0]);

    // Kinh độ
    char lon_raw[15] = {0}, lon_dir[2] = {0};
    get_field(sentence, 4, lon_raw, sizeof(lon_raw));
    get_field(sentence, 5, lon_dir, sizeof(lon_dir));
    data->longitude = nmea_to_decimal(lon_raw, lon_dir[0]);

    // Chất lượng Fix
    get_field(sentence, 6, field, sizeof(field));
    int fix = atoi(field);
    data->fix_quality = (neo6m_fix_quality_t)fix;
    // Luon cap nhat is_valid theo trang thai fix hien tai (ca khi mat fix),
    // tránh nhầm lẫn dữ liệu cũ khi mất fix. Nếu fix = 0 (invalid) -> is_valid = false.
    data->is_valid = (fix > 0);

    // Số lượng vệ tinh kết nối
    get_field(sentence, 7, field, sizeof(field));
    data->satellites = (uint8_t)atoi(field);

    // Sai số hình học HDOP
    get_field(sentence, 8, field, sizeof(field));
    data->hdop = atof(field);

    // Độ cao mặt nước biển
    get_field(sentence, 9, field, sizeof(field));
    data->altitude_m = atof(field);

    return NEO6M_OK;
}

// Xử lý gói tin mẫu $GPRMC (Vị trí, Vận tốc, Hướng di chuyển, Ngày tháng)
NEO6M_Status NEO6M_ParseRMC(const char *sentence, neo6m_data_t *data) {
    if (sentence == NULL || data == NULL) return NEO6M_ERROR;
    if (strncmp(sentence, NEO6M_NMEA_RMC, 6) != 0) return NEO6M_ERROR;
    if (!NEO6M_ValidateChecksum(sentence)) return NEO6M_ERROR;

    char field[20];

    get_field(sentence, 2, field, sizeof(field));
    if (field[0] != 'A') {
        data->is_valid = false;
        return NEO6M_OK;
    }

    get_field(sentence, 1, field, sizeof(field));
    if (field_len_ok(field, 6)) {
        char tmp[3] = {0};
        tmp[0] = field[0]; tmp[1] = field[1]; data->hour   = (uint8_t)atoi(tmp);
        tmp[0] = field[2]; tmp[1] = field[3]; data->minute = (uint8_t)atoi(tmp);
        tmp[0] = field[4]; tmp[1] = field[5]; data->second = (uint8_t)atoi(tmp);
    }

    char lat_raw[15] = {0}, lat_dir[2] = {0};
    get_field(sentence, 3, lat_raw, sizeof(lat_raw));
    get_field(sentence, 4, lat_dir, sizeof(lat_dir));
    data->latitude = nmea_to_decimal(lat_raw, lat_dir[0]);

    char lon_raw[15] = {0}, lon_dir[2] = {0};
    get_field(sentence, 5, lon_raw, sizeof(lon_raw));
    get_field(sentence, 6, lon_dir, sizeof(lon_dir));
    data->longitude = nmea_to_decimal(lon_raw, lon_dir[0]);

    get_field(sentence, 7, field, sizeof(field));
    data->speed_knots = atof(field);
    data->speed_kmh   = data->speed_knots * 1.852f; 

    get_field(sentence, 8, field, sizeof(field));
    data->course_deg = atof(field);

    // Ngày tháng năm thực tế
    get_field(sentence, 9, field, sizeof(field));
    if (field_len_ok(field, 6)) {
        char tmp[3] = {0};
        tmp[0] = field[0]; tmp[1] = field[1]; data->day   = (uint8_t)atoi(tmp);
        tmp[0] = field[2]; tmp[1] = field[3]; data->month = (uint8_t)atoi(tmp);
        tmp[0] = field[4]; tmp[1] = field[5]; data->year  = (uint16_t)(atoi(tmp) + 2000);
    }

    data->is_valid = true;
    return NEO6M_OK;
}
// Hàm Update có timeout (mặc định 250ms)
NEO6M_Status NEO6M_Update(neo6m_dev_t *dev, neo6m_data_t *data) {
    return NEO6M_Update_WithTimeout(dev, data, NEO6M_UART_TIMEOUT_MS);
}

// Hàm chính hỗ trợ timeout
NEO6M_Status NEO6M_Update_WithTimeout(neo6m_dev_t *dev, neo6m_data_t *data, uint32_t timeout_ms) {
    if (dev == NULL || data == NULL) return NEO6M_ERROR;

    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
    bool got_gga = false;
    bool got_rmc = false;

    while (!time_reached(deadline)) {
        if (NEO6M_ReadLine(dev) == NEO6M_OK) {
            const char *sentence = dev->nmea_buf;

            if (strncmp(sentence, NEO6M_NMEA_GGA, 6) == 0) {
                if (NEO6M_ParseGGA(sentence, data) == NEO6M_OK) {
                    got_gga = true;
                }
            } else if (strncmp(sentence, NEO6M_NMEA_RMC, 6) == 0) {
                if (NEO6M_ParseRMC(sentence, data) == NEO6M_OK) {
                    got_rmc = true;
                }
            }

            if (got_gga && got_rmc) return NEO6M_OK;   // Đủ dữ liệu → thoát ngay
        }
        // Không dùng vTaskDelay ở đây vì ReadLine đã nhường CPU khi chờ UART,
        // giúp đọc ngay bản tin tiếp theo nếu có sẵn trong buffer.
    }

    return (got_gga || got_rmc) ? NEO6M_OK : NEO6M_TIMEOUT;
}
#include "neo6m.h"
#include <stdio.h>

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

// Hàm bổ trợ tách chuỗi con theo dấu phẩy NMEA
static void get_field(const char *sentence, int index, char *out, uint8_t out_size) {
    out[0] = '\0';
    int  field  = 0;
    int  i      = 0;
    int  out_i  = 0;

    while (sentence[i] != '\0' && field <= index) {
        if (sentence[i] == ',') {
            if (field == index) break;
            field++;
            i++;
            out_i = 0;
            continue;
        }
        if (field == index && out_i < (out_size - 1)) {
            out[out_i++] = sentence[i];
        }
        i++;
    }
    out[out_i] = '\0';
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

    sleep_ms(500);

    dev->is_initialized = true;
    printf("[NEO6M] Khoi tao UART%d OK - Baud: %lu\n", uart_get_index(dev->handle_uart), (unsigned long)dev->baudrate);

    return NEO6M_OK;
}

// Đọc dòng ký tự liên tục từ UART cho đến khi gặp ký tự xuống dòng '\n'
NEO6M_Status NEO6M_ReadLine(neo6m_dev_t *dev) {
    if (dev == NULL || dev->handle_uart == NULL) return NEO6M_ERROR;

    dev->buf_idx = 0;
    memset(dev->nmea_buf, 0, NEO6M_NMEA_MAX_LEN);

    bool started = false;
    absolute_time_t t_deadline = make_timeout_time_ms(NEO6M_UART_TIMEOUT_MS);

    while (!time_reached(t_deadline)) {
        if (!uart_is_readable(dev->handle_uart)) {
            sleep_us(100);
            continue;
        }

        char c = uart_getc(dev->handle_uart);

        if (!started) {
            if (c == NEO6M_NMEA_START) {
                dev->nmea_buf[0] = c;
                dev->buf_idx     = 1;
                started          = true;
                t_deadline = make_timeout_time_ms(NEO6M_UART_TIMEOUT_MS);
            }
            continue;
        }

        if (c == NEO6M_NMEA_END_LF) {
            dev->nmea_buf[dev->buf_idx] = '\0';
            return NEO6M_OK;
        }

        if (c == NEO6M_NMEA_END_CR) continue;

        if (dev->buf_idx < (NEO6M_NMEA_MAX_LEN - 1)) {
            dev->nmea_buf[dev->buf_idx++] = c;
        } else {
            started      = false;
            dev->buf_idx = 0;
        }
    }

    return NEO6M_ERROR;
}

// Xử lý gói tin mẫu $GPGGA (Vị trí, Cao độ, Số lượng vệ tinh)
NEO6M_Status NEO6M_ParseGGA(const char *sentence, neo6m_data_t *data) {
    if (sentence == NULL || data == NULL) return NEO6M_ERROR;
    if (strncmp(sentence, NEO6M_NMEA_GGA, 6) != 0) return NEO6M_ERROR;
    if (!NEO6M_ValidateChecksum(sentence)) return NEO6M_ERROR;

    char field[20];

    // Thời gian UTC
    get_field(sentence, 1, field, sizeof(field));
    if (field[0] != '\0') {
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
    if (fix > 0) {
        data->is_valid = true;
    }

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
    if (field[0] != '\0') {
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
    if (field[0] != '\0') {
        char tmp[3] = {0};
        tmp[0] = field[0]; tmp[1] = field[1]; data->day   = (uint8_t)atoi(tmp);
        tmp[0] = field[2]; tmp[1] = field[3]; data->month = (uint8_t)atoi(tmp);
        tmp[0] = field[4]; tmp[1] = field[5]; data->year  = (uint16_t)(atoi(tmp) + 2000);
    }

    data->is_valid = true;
    return NEO6M_OK;
}

// Hàm wrapper nhận diện và phân phối luồng xử lý NMEA chính xác
NEO6M_Status NEO6M_Update(neo6m_dev_t *dev, neo6m_data_t *data) {
    if (dev == NULL || data == NULL) return NEO6M_ERROR;

    if (NEO6M_ReadLine(dev) != NEO6M_OK) return NEO6M_ERROR;

    const char *sentence = dev->nmea_buf;

    if (strncmp(sentence, NEO6M_NMEA_GGA, 6) == 0) {
        return NEO6M_ParseGGA(sentence, data);
    }
    if (strncmp(sentence, NEO6M_NMEA_RMC, 6) == 0) {
        return NEO6M_ParseRMC(sentence, data);
    }

    return NEO6M_OK; 
}
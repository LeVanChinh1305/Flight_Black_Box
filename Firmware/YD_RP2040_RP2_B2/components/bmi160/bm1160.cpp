#include "bmi160.h"
#include <stdio.h>



//  các hàm giao tiếp nội bộ qua I2C

static BMI_Status BMI_Write_Reg(bmi_dev_t *dev, uint8_t reg_addr, uint8_t data) {
    if (dev == NULL || dev->handle_i2c == NULL) return BMI_ERROR;

    uint8_t write_buf[2];
    write_buf[0] = reg_addr; // Giao tiếp I2C không cần mask bit như SPI
    write_buf[1] = data;

    // Gửi địa chỉ thanh ghi kèm dữ liệu cần ghi
    int ret = i2c_write_blocking(dev->handle_i2c, dev->i2c_addr, write_buf, 2, false);
    
    return (ret >= 0) ? BMI_OK : BMI_ERROR;
}

static BMI_Status BMI_Read_Reg(bmi_dev_t *dev, uint8_t reg_addr, uint8_t *data, uint16_t len) {
    if (dev == NULL || dev->handle_i2c == NULL) return BMI_ERROR;

    // Bước 1: Gửi địa chỉ thanh ghi muốn đọc (nostop = true để giữ bus)
    int ret = i2c_write_blocking(dev->handle_i2c, dev->i2c_addr, &reg_addr, 1, true);
    if (ret < 0) return BMI_ERROR;

    // Bước 2: Đọc dữ liệu trả về từ cảm biến
    ret = i2c_read_blocking(dev->handle_i2c, dev->i2c_addr, data, len, false);

    return (ret >= 0) ? BMI_OK : BMI_ERROR;
}


//  các hàm kiểm tra kết nối và gửi lệnh 

BMI_Status BMI160_ReadID(bmi_dev_t *dev, uint8_t *id) {
    if (dev == NULL || dev->handle_i2c == NULL) return BMI_ERROR;
    return BMI_Read_Reg(dev, BMI_CHIP_ID, id, 1);
}

BMI_Status BMI160_CheckConnection(bmi_dev_t *dev) {
    if (dev == NULL || dev->handle_i2c == NULL) return BMI_ERROR;
    uint8_t id = 0;
    if (BMI160_ReadID(dev, &id) == BMI_OK) {
        if (id == BMI_CHIPID_VALUE) return BMI_OK;
    }
    return BMI_ERROR;
}

BMI_Status BMI160_SendCommand(bmi_dev_t *dev, uint8_t cmd) {
    if (dev == NULL || dev->handle_i2c == NULL) return BMI_ERROR;
    return BMI_Write_Reg(dev, BMI_CMD, cmd);
}


//  hàm đọc trạng thái và lỗi 

BMI_Status BMI160_ReadStatus(bmi_dev_t *dev, uint8_t *status) {
    if (dev == NULL || dev->handle_i2c == NULL) return BMI_ERROR;
    return BMI_Read_Reg(dev, BMI_STATUS, status, 1);
}

BMI_Status BMI160_ReadError(bmi_dev_t *dev, uint8_t *error) {
    if (dev == NULL || dev->handle_i2c == NULL) return BMI_ERROR;
    return BMI_Read_Reg(dev, BMI_ERR_REG, error, 1);
}


//  hàm cấu hình cảm biến 

BMI_Status BMI160_ConfigAccel(bmi_dev_t *dev, uint8_t config, uint8_t range) {
    if (dev == NULL || dev->handle_i2c == NULL) return BMI_ERROR;
    BMI_Status status;
    status = BMI_Write_Reg(dev, BMI_ACC_CONFIG, config);
    if (status != BMI_OK) return status;
    status = BMI_Write_Reg(dev, BMI_ACC_RANGE, range);
    return status;
}

BMI_Status BMI160_ConfigGyro(bmi_dev_t *dev, uint8_t config, uint8_t range) {
    if (dev == NULL || dev->handle_i2c == NULL) return BMI_ERROR;
    BMI_Status status;
    status = BMI_Write_Reg(dev, BMI_GYR_CONFIG, config);
    if (status != BMI_OK) return status;
    status = BMI_Write_Reg(dev, BMI_GYR_RANGE, range);
    return status;
}

BMI_Status BMI160_Config(bmi_dev_t *dev) {
    if (dev == NULL || dev->handle_i2c == NULL) return BMI_ERROR;
    BMI_Status status;
    status = BMI160_ConfigAccel(dev, dev->config.accel_odr, dev->config.accel_range);
    if (status != BMI_OK) return status;
    status = BMI160_ConfigGyro(dev, dev->config.gyro_odr, dev->config.gyro_range);
    return status;
}


//  hàm khởi tạo cảm biến 
// Gắn con trỏ I2C --> Cấu hình GPIO --> Reset chip --> Đọc kiểm tra ID --> Bật nguồn cảm biến --> Ghi cấu hình vào struct và cấu hình thanh ghi.

BMI_Status BMI160_Init(bmi_dev_t *dev, i2c_inst_t *handle_i2c, uint8_t i2c_addr, const BMI160_Config_t *config) {
    if (dev == NULL || handle_i2c == NULL || config == NULL) return BMI_ERROR;

    // gắn con trỏ I2C và địa chỉ slave
    dev->handle_i2c    = handle_i2c;
    dev->i2c_addr       = i2c_addr;
    dev->is_initialized = false;

    // Khởi tạo giao tiếp I2C phần cứng (Tốc độ tiêu chuẩn 400 kHz Fast Mode)
    i2c_init(dev->handle_i2c, 400000);

    // cấu hình chức năng chân GPIO cho I2C
    gpio_set_function(BMI160_PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(BMI160_PIN_SCL, GPIO_FUNC_I2C);
    
    // Kích hoạt điện trở kéo lên trong RP2040 (Nên có pull-up ngoài 4.7k trên module mạch cứng)
    gpio_pull_up(BMI160_PIN_SDA);
    gpio_pull_up(BMI160_PIN_SCL);

    sleep_ms(10); // chờ nguồn ổn định

    // reset chip 
    BMI160_SendCommand(dev, BMI_CMD_SOFT_RESET);
    sleep_ms(15); // cần delay ít nhất 15ms sau khi reset theo Datasheet

    // Đọc thử để chip ổn định lại giao tiếp sau Reset
    uint8_t temp_id = 0;
    BMI160_ReadID(dev, &temp_id);
    sleep_ms(2); 

    // kiểm tra id 
    if (BMI160_CheckConnection(dev) != BMI_OK) {
        printf("[BMI160] Loi: CHIP_ID khong khop hoac khong tim thay thiet bi I2C!\n");
        return BMI_ERROR;
    }
    printf("[BMI160] Ket noi I2C OK.\n");

    // bật nguồn cảm biến 
    BMI160_SendCommand(dev, BMI_CMD_ACC_NET_NORMAL);
    sleep_ms(BMI_DELAY_ACC_PMU_MS);
    BMI160_SendCommand(dev, BMI_CMD_GYR_NET_NORMAL);
    sleep_ms(BMI_DELAY_GYRO_PMU_MS);

    // ghi cấu hình vào struct và cấu hình thanh ghi
    dev->config = *config;
    if (BMI160_Config(dev) != BMI_OK) {
        return BMI_ERROR;
    }

    // kết thúc
    dev->is_initialized = true;
    printf("[BMI160] Khoi tao hoan tat.\n");
    return BMI_OK;
}


//  hàm đọc dữ liệu cảm biến 

BMI_Status BMI160_ReadData(bmi_dev_t *dev, BMI160_Data *data) {
    if (dev == NULL || dev->handle_i2c == NULL) return BMI_ERROR;
    uint8_t raw_data[12];

    // Đọc liên tục 12 byte bắt đầu từ thanh ghi BMI_GYR_X_LSB (0x0C)
    BMI_Status status = BMI_Read_Reg(dev, BMI_GYR_X_LSB, raw_data, 12);

    if (status == BMI_OK) {
        // Ghép byte cho Gyro (Byte 0 đến 5)
        data->gyr_x = (int16_t)((raw_data[1] << 8) | raw_data[0]);
        data->gyr_y = (int16_t)((raw_data[3] << 8) | raw_data[2]);
        data->gyr_z = (int16_t)((raw_data[5] << 8) | raw_data[4]);

        // Ghép byte cho Accel (Byte 6 đến 11)
        data->acc_x = (int16_t)((raw_data[7]  << 8) | raw_data[6]);
        data->acc_y = (int16_t)((raw_data[9]  << 8) | raw_data[8]);
        data->acc_z = (int16_t)((raw_data[11] << 8) | raw_data[10]);
    }

    return status;
}

BMI_Status BMI160_ReadTemperature(bmi_dev_t *dev, int16_t *temp) {
    if (dev == NULL || dev->handle_i2c == NULL) return BMI_ERROR;
    uint8_t raw_data[2];
    BMI_Status status = BMI_Read_Reg(dev, BMI_TEMP_LSB, raw_data, 2);

    if (status == BMI_OK) {
        // Ghép LSB và MSB
        *temp = (int16_t)((raw_data[1] << 8) | raw_data[0]);
    }
    return status;
}


//  ================================  các hàm FIFO  ================================

BMI_Status BMI160_FIFO_Config(bmi_dev_t *dev, const BMI160_FIFO_Config_t *fifo_cfg) {
    if (dev == NULL || dev->handle_i2c == NULL || fifo_cfg == NULL) return BMI_ERROR;

    // FIFO_CONFIG[0] (0x46) chỉ chứa fifo_water_mark<7:0>, đơn vị 4 byte
    BMI_Status status = BMI_Write_Reg(dev, BMI_FIFO_CONFIG_0, fifo_cfg->watermark);
    if (status != BMI_OK) return status;

    // FIFO_CONFIG[1] (0x47): gyr_en | acc_en | mag_en | header_en | tag_int1 | tag_int2 | time_en | reserved
    uint8_t cfg1 = 0;
    if (fifo_cfg->gyr_en)    cfg1 |= BMI_FIFO_GYR_EN_MASK;
    if (fifo_cfg->acc_en)    cfg1 |= BMI_FIFO_ACC_EN_MASK;
    if (fifo_cfg->header_en) cfg1 |= BMI_FIFO_HEADER_EN_MASK;
    if (fifo_cfg->time_en)   cfg1 |= BMI_FIFO_TIME_EN_MASK;

    status = BMI_Write_Reg(dev, BMI_FIFO_CONFIG_1, cfg1);
    if (status != BMI_OK) return status;

    // Lưu lại cấu hình để hàm parse sau này biết cách giải mã buffer (header hay headerless)
    dev->fifo_config = *fifo_cfg;

    // Đổi cấu hình FIFO nên đi kèm flush để tránh lẫn dữ liệu cũ với format mới
    return BMI160_FIFO_Flush(dev);
}

BMI_Status BMI160_FIFO_GetLength(bmi_dev_t *dev, uint16_t *length) {
    if (dev == NULL || dev->handle_i2c == NULL || length == NULL) return BMI_ERROR;

    uint8_t raw[2];
    // FIFO_LENGTH_1 [2:0] chứa 3 bit cao, FIFO_LENGTH_0 chứa 8 bit thấp -> tối đa 0x7FF = 2047 nhưng FIFO vật lý chỉ 1024 byte
    BMI_Status status = BMI_Read_Reg(dev, BMI_FIFO_LENGTH_0, raw, 2);
    if (status != BMI_OK) return status;

    uint16_t len = ((uint16_t)(raw[1] & 0x07) << 8) | raw[0];
    if (len > BMI_FIFO_BUFFER_SIZE) len = BMI_FIFO_BUFFER_SIZE; // phòng hờ dữ liệu nhiễu trên bus
    *length = len;
    return BMI_OK;
}

BMI_Status BMI160_FIFO_Read(bmi_dev_t *dev, uint8_t *buffer, uint16_t len) {
    if (dev == NULL || dev->handle_i2c == NULL || buffer == NULL || len == 0) return BMI_ERROR;
    // FIFO_DATA (0x24) hỗ trợ burst read liên tục, mỗi lần đọc thêm sẽ tự động trả byte kế tiếp trong FIFO
    return BMI_Read_Reg(dev, BMI_FIFO_DATA, buffer, len);
}

BMI_Status BMI160_FIFO_Flush(bmi_dev_t *dev) {
    if (dev == NULL || dev->handle_i2c == NULL) return BMI_ERROR;
    return BMI160_SendCommand(dev, BMI_CMD_FIFO_FLUSH);
}

// hàm nội bộ: đọc 6 byte gyro hoặc accel dạng LSB-first, giống thứ tự trong DATA register
static inline void BMI_ParseAxes(const uint8_t *p, int16_t *x, int16_t *y, int16_t *z) {
    *x = (int16_t)((p[1] << 8) | p[0]);
    *y = (int16_t)((p[3] << 8) | p[2]);
    *z = (int16_t)((p[5] << 8) | p[4]);
}

BMI160_FIFO_Result_t BMI160_FIFO_ParseFrames(bmi_dev_t *dev, const uint8_t *buffer, uint16_t buf_len,
                                              BMI160_FIFO_Frame_t *frames, uint16_t max_frames) {
    BMI160_FIFO_Result_t result = {0, 0, 0, false};
    if (dev == NULL || buffer == NULL || frames == NULL || max_frames == 0) return result;

    uint16_t idx = 0;

    if (!dev->fifo_config.header_en) {
        // ---------- Headerless mode: chỉ có dữ liệu, không header, ODR acc/gyr phải giống nhau ----------
        uint16_t frame_len = 0;
        if (dev->fifo_config.acc_en) frame_len += BMI_FIFO_FRAME_LEN_ACC;
        if (dev->fifo_config.gyr_en) frame_len += BMI_FIFO_FRAME_LEN_GYR;
        if (frame_len == 0) { result.bytes_read = 0; return result; }

        while (idx + frame_len <= buf_len && result.frame_count < max_frames) {
            // Overread: byte đầu của phần dữ liệu không hợp lệ sẽ là 0x80 lặp lại
            if (buffer[idx] == BMI_FIFO_HDR_INVALID && buffer[idx + 1] == BMI_FIFO_HDR_INVALID) break;

            BMI160_FIFO_Frame_t f = {0};
            uint16_t off = idx;
            // Thứ tự vật lý trong FIFO/DATA register luôn là: gyro trước, accel sau (giống thanh ghi 0x0C..0x17)
            if (dev->fifo_config.gyr_en) {
                BMI_ParseAxes(&buffer[off], &f.gyr_x, &f.gyr_y, &f.gyr_z);
                f.has_gyr = true;
                off += BMI_FIFO_FRAME_LEN_GYR;
            }
            if (dev->fifo_config.acc_en) {
                BMI_ParseAxes(&buffer[off], &f.acc_x, &f.acc_y, &f.acc_z);
                f.has_acc = true;
                off += BMI_FIFO_FRAME_LEN_ACC;
            }
            frames[result.frame_count++] = f;
            idx += frame_len;
        }
        result.bytes_read = idx;
        return result;
    }

    // ---------- Header mode: mỗi frame bắt đầu bằng 1 byte header ----------
    while (idx < buf_len && result.frame_count < max_frames) {
        uint8_t header = buffer[idx];

        if (header == BMI_FIFO_HDR_INVALID) {
            // Hết dữ liệu hợp lệ trong buffer đã đọc
            break;
        }

        uint8_t mode = header & BMI_FIFO_HDR_MODE_MASK;
        uint8_t parm = (header & BMI_FIFO_HDR_PARM_MASK) >> BMI_FIFO_HDR_PARM_SHIFT;

        if (mode == BMI_FIFO_HDR_MODE_CONTROL) {
            // ---- control frame: skip / sensortime / input_config ----
            if (parm == BMI_FIFO_CTRL_SKIP_FRAME) {
                if (idx + 2 > buf_len) break; // chưa đủ dữ liệu, dừng để lần đọc sau xử lý tiếp
                result.skipped_frames += buffer[idx + 1];
                result.overflow_detected = true;
                idx += 2; // header + 1 byte số frame bị skip
            } else if (parm == BMI_FIFO_CTRL_SENSORTIME) {
                if (idx + 4 > buf_len) break; // header + 3 byte sensortime
                idx += 4;
                break; // sensortime luôn là frame cuối cùng hợp lệ -> dừng parse
            } else if (parm == BMI_FIFO_CTRL_INPUT_CONFIG) {
                if (idx + 2 > buf_len) break; // header + 1 byte config-change info
                idx += 2;
            } else {
                // opcode không xác định -> không thể biết độ dài, dừng an toàn
                break;
            }
            continue;
        }

        if (mode != BMI_FIFO_HDR_MODE_REGULAR) {
            // fh_mode reserved (0b00 / 0b11) không hợp lệ, dừng parse để tránh đọc lệch offset
            break;
        }

        // ---- regular frame: dữ liệu cảm biến, có thể là acc/gyr/mag riêng lẻ hoặc kết hợp ----
        bool has_gyr = (parm & BMI_FIFO_PARM_GYR_BIT) != 0;
        bool has_acc = (parm & BMI_FIFO_PARM_ACC_BIT) != 0;
        bool has_mag = (parm & BMI_FIFO_PARM_MAG_BIT) != 0; // driver này chưa hỗ trợ mag, chỉ dùng để tính offset

        uint16_t data_len = (has_mag ? 8 : 0) + (has_gyr ? BMI_FIFO_FRAME_LEN_GYR : 0) + (has_acc ? BMI_FIFO_FRAME_LEN_ACC : 0);
        if (idx + 1 + data_len > buf_len) break; // chưa đủ byte trong buffer, chờ lần đọc sau

        BMI160_FIFO_Frame_t f = {0};
        uint16_t off = idx + 1;
        if (has_mag) off += 8; // bỏ qua dữ liệu mag (chưa parse) - đúng thứ tự mag -> gyro -> accel
        if (has_gyr) {
            BMI_ParseAxes(&buffer[off], &f.gyr_x, &f.gyr_y, &f.gyr_z);
            f.has_gyr = true;
            off += BMI_FIFO_FRAME_LEN_GYR;
        }
        if (has_acc) {
            BMI_ParseAxes(&buffer[off], &f.acc_x, &f.acc_y, &f.acc_z);
            f.has_acc = true;
            off += BMI_FIFO_FRAME_LEN_ACC;
        }

        frames[result.frame_count++] = f;
        idx += 1 + data_len;
    }

    result.bytes_read = idx;
    return result;
}

BMI_Status BMI160_FIFO_ReadAndParse(bmi_dev_t *dev, uint8_t *scratch_buf, uint16_t scratch_buf_size,
                                     BMI160_FIFO_Frame_t *frames, uint16_t max_frames,
                                     BMI160_FIFO_Result_t *result) {
    if (dev == NULL || scratch_buf == NULL || frames == NULL || result == NULL) return BMI_ERROR;

    uint16_t fifo_len = 0;
    BMI_Status status = BMI160_FIFO_GetLength(dev, &fifo_len);
    if (status != BMI_OK) return status;

    if (fifo_len == 0) {
        *result = (BMI160_FIFO_Result_t){0, 0, 0, false};
        return BMI_OK;
    }
    if (fifo_len > scratch_buf_size) fifo_len = scratch_buf_size; // tránh tràn buffer người dùng cấp phát

    status = BMI160_FIFO_Read(dev, scratch_buf, fifo_len);
    if (status != BMI_OK) return status;

    *result = BMI160_FIFO_ParseFrames(dev, scratch_buf, fifo_len, frames, max_frames);
    return BMI_OK;
}
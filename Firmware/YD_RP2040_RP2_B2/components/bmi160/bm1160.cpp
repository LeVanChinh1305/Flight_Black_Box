#include "bmi160.h"
#include <stdio.h>

//  bảng tra cứu thay thế HAL_Delay() 
// STM32: HAL_Delay(ms)   →   RP2040: sleep_ms(ms)


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
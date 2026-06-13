#include "BMI160.h"

#define HAL_MAX_DELAY 1000

// các hàm giao tiếp nội bộ 
static HAL_StatusTypeDef BMI_Write_Reg(bmi_dev_t *dev, uint8_t reg_addr, uint8_t data){
    if (dev == NULL || dev->handle_spi == NULL) return HAL_ERROR;
    uint8_t write_buf[2] = {reg_addr, data};
    write_buf[0] = reg_addr & BMI160_SPI_WRITE_MASK; // phép AND để đảm bảo bit ghi được đặt về 0
    write_buf[1] = data;
    HAL_StatusTypeDef status;
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET); // kéo CS xuống để bắt đầu giao tiếp

    status = HAL_SPI_Transmit(dev->handle_spi, write_buf, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET); // thả CS lên để kết thúc giao tiếp
    return status;
}


static HAL_StatusTypeDef BMI_Read_Reg(bmi_dev_t *dev, uint8_t reg_addr, uint8_t *data, uint16_t len){
    if (dev == NULL || dev->handle_spi == NULL) return HAL_ERROR;
    // Bit 7 = 1 để báo hiệu lệnh Đọc (Read)
    uint8_t tx_data = reg_addr | BMI160_SPI_READ_MASK;
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET); // kéo CS xuống để bắt đầu giao tiếp
    HAL_StatusTypeDef status;
    status = HAL_SPI_Transmit(dev->handle_spi, &tx_data, 1, HAL_MAX_DELAY);
    // Nhận dữ liệu trả về
    if (status == HAL_OK) {
        status = HAL_SPI_Receive(dev->handle_spi, data, len, HAL_MAX_DELAY);
    }
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET); // thả CS lên để kết thúc giao tiếp
    return status;
}


// các hàm kiểm tra kết nối và gửi lệnh 
HAL_StatusTypeDef BMI160_ReadID(bmi_dev_t *dev, uint8_t *id){
    if (dev == NULL || dev->handle_spi == NULL) return HAL_ERROR;
    return BMI_Read_Reg(dev, BMI_CHIP_ID, id, 1); 
    // mục đích : trả về id đọc được 
}
HAL_StatusTypeDef BMI160_CheckConnection(bmi_dev_t *dev){
    if (dev == NULL || dev->handle_spi == NULL) return HAL_ERROR;
    uint8_t id = 0; 
    if(BMI160_ReadID(dev, &id) == HAL_OK){
        if(id == BMI_CHIPID_VALUE) return HAL_OK;
    }
    return HAL_ERROR; 
    // mục đích : kiểm tra có đúng chip hay không 
}
HAL_StatusTypeDef BMI160_SendCommand(bmi_dev_t *dev, uint8_t cmd){
    if (dev == NULL || dev->handle_spi == NULL) return HAL_ERROR;
    return BMI_Write_Reg(dev, BMI_CMD, cmd); 
    // mục đích : ghi 1 lệnh vào 1 thanh ghi 
}



// hàm đọc trạng thái và lỗi 
HAL_StatusTypeDef BMI160_ReadStatus(bmi_dev_t *dev, uint8_t *status) {
    if (dev == NULL || dev->handle_spi == NULL) return HAL_ERROR;
    return BMI_Read_Reg(dev, BMI_STATUS, status, 1);
}
HAL_StatusTypeDef BMI160_ReadError(bmi_dev_t *dev, uint8_t *error) {
    if (dev == NULL || dev->handle_spi == NULL) return HAL_ERROR;
    return BMI_Read_Reg(dev, BMI_ERR_REG, error, 1);
}



// hàm cấu hình cảm biến bao gồm cấu hình con quay hồi chuyển và gia tốc kế 
HAL_StatusTypeDef BMI160_ConfigAccel(bmi_dev_t *dev, uint8_t config, uint8_t range){
    if (dev == NULL || dev->handle_spi == NULL) return HAL_ERROR;
    HAL_StatusTypeDef status;
    status = BMI_Write_Reg(dev, BMI_ACC_CONFIG, config);
    if(status != HAL_OK) return status;
    status = BMI_Write_Reg(dev, BMI_ACC_RANGE, range);
    return status; 
}
HAL_StatusTypeDef BMI160_ConfigGyro(bmi_dev_t *dev, uint8_t config, uint8_t range){
    if (dev == NULL || dev->handle_spi == NULL) return HAL_ERROR;
    HAL_StatusTypeDef status;
    status = BMI_Write_Reg(dev, BMI_GYR_CONFIG, config);
    if(status != HAL_OK) return status;
    status = BMI_Write_Reg(dev, BMI_GYR_RANGE, range);
    return status; 
}

HAL_StatusTypeDef BMI160_Config(bmi_dev_t *dev){
    if (dev == NULL || dev->handle_spi == NULL) return HAL_ERROR;
    HAL_StatusTypeDef status;
    status = BMI160_ConfigAccel(dev, dev->config.accel_odr, dev->config.accel_range);
    if(status != HAL_OK) return status; 
    status = BMI160_ConfigGyro(dev, dev->config.gyro_odr, dev->config.gyro_range);
    return status; 
}


// hàm khởi tạo cảm biến
// Gắn con trỏ SPI --> Reset chip --> Kiểm tra ID --> Bật nguồn cảm biến --> Ghi cấu hình vào struct và cấu hình thanh ghi.
HAL_StatusTypeDef BMI160_Init(bmi_dev_t *dev, SPI_HandleTypeDef *handle_spi, GPIO_TypeDef *cs_port, uint16_t cs_pin, const BMI160_Config_t *config){
    if (dev == NULL || handle_spi == NULL || config == NULL) return HAL_ERROR;
    // gắn con trỏ SPI
    dev->handle_spi = handle_spi; 
    dev->cs_port = cs_port;
    dev->cs_pin = cs_pin;
    dev->is_initialized = false;

    // reset chip 
    BMI160_SendCommand(dev, BMI_CMD_SOFT_RESET);
    HAL_Delay(15); // Cần delay ít nhất 15ms sau khi reset theo Datasheet

    // kiểm tra id 
    if(BMI160_CheckConnection(dev) != HAL_OK){
        return HAL_ERROR;
    }

    // bật nguồn cảm biến 
    BMI160_SendCommand(dev, BMI_CMD_ACC_NET_NORMAL);
    HAL_Delay(BMI_DELAY_ACC_PMU_MS); 
    BMI160_SendCommand(dev, BMI_CMD_GYR_NET_NORMAL);
    HAL_Delay(BMI_DELAY_GYRO_PMU_MS);

    // Ghi cấu hình vào struct và cấu hình thanh ghi
    dev->config = *config;
    // cấu hình xuống thanh ghi 
    if(BMI160_Config(dev)!= HAL_OK){
        return HAL_ERROR;
    }

    // kết thúc
    dev->is_initialized = true;
    return HAL_OK;

}


// hàm đọc dữ liệu cảm biến, bao gồm gia tốc kế, con quay hồi chuyển và nhiệt độ
HAL_StatusTypeDef BMI160_ReadData(bmi_dev_t *dev, BMI160_Data *data) {
    if (dev == NULL || dev->handle_spi == NULL) return HAL_ERROR;
    uint8_t raw_data[12];
    
    // Đọc liên tục 12 byte bắt đầu từ thanh ghi BMI_GYR_X_LSB (0x0C)
    HAL_StatusTypeDef status = BMI_Read_Reg(dev, BMI_GYR_X_LSB, raw_data, 12);
    
    if (status == HAL_OK) {
        // Ghép byte cho Gyro (Byte 0 đến 5)
        data->gyr_x = (int16_t)((raw_data[1] << 8) | raw_data[0]);
        data->gyr_y = (int16_t)((raw_data[3] << 8) | raw_data[2]);
        data->gyr_z = (int16_t)((raw_data[5] << 8) | raw_data[4]);
        
        // Ghép byte cho Accel (Byte 6 đến 11)
        data->acc_x = (int16_t)((raw_data[7] << 8)  | raw_data[6]);
        data->acc_y = (int16_t)((raw_data[9] << 8)  | raw_data[8]);
        data->acc_z = (int16_t)((raw_data[11] << 8) | raw_data[10]);
    }
    
    return status;
}

HAL_StatusTypeDef BMI160_ReadTemperature(bmi_dev_t *dev, int16_t *temp) {
    if (dev == NULL || dev->handle_spi == NULL) return HAL_ERROR;
    uint8_t raw_data[2];
    HAL_StatusTypeDef status = BMI_Read_Reg(dev, BMI_TEMP_LSB, raw_data, 2);
    
    if (status == HAL_OK) {
        // Ghép LSB và MSB
        *temp = (int16_t)((raw_data[1] << 8) | raw_data[0]);
    }
    return status;
}


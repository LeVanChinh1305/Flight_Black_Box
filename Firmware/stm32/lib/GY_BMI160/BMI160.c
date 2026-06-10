#include "BMI160.h"

#define HAL_MAX_DELAY 1000

// các hàm giao tiếp nội bộ 
static HAL_StatusTypeDef BMI_WRITE_REG(bmi_dev_t *a, uint8_t reg_addr, uint8_t data){
    uint8_t write_buf[2] = {reg_addr, data};
    write_buf[0] = reg_addr & BMI160_SPI_WRITE_BIT; // phép AND để đảm bảo bit ghi được đặt về 0
    write_buf[1] = data;
    HAL_StatusTypeDef status;
    BMI160_CS_LOW(); // kéo CS xuống để bắt đầu giao tiếp

    status = HAL_SPI_Transmit(a->i2c_handle, write_buf, 2, HAL_MAX_DELAY);
    BMI160_CS_HIGH(); // thả CS lên để kết thúc giao tiếp
    return status;
}


static HAL_StatusTypeDef BMI_READ_REG(bmi_dev_t *a, uint8_t reg_addr, uint8_t *data){

}
// hàm khởi tạo cảm biến



// hàm cấu hình cảm biến bao gồm cấu hình con quay hồi chuyển và gia tốc kế 



// hàm kiểm tra kết nối cảm biến



// hàm đọc ID cảm biến


// hàm đọc dữ liệu cảm biến, bao gồm gia tốc kế, con quay hồi chuyển và nhiệt độ

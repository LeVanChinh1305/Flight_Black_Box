#ifndef BMI160_H
#define BMI160_H

#include "stm32f1xx_hal.h"
#include "stdint.h"
#include "stdbool.h"

// địa chỉ của cảm biến trên bus SPI
#define BMI160_CS_PORT GPIOA // chân CS của cảm biến được kết nối với chân PA3 trên STM32
#define BMI160_CS_PIN  GPIO_PIN_3
#define BMI160_SPI_READ_BIT  0x80
#define BMI160_SPI_WRITE_BIT 0x7F

// macro để điều khiển chân CS của cảm biến 
#define BMI160_CS_LOW()  HAL_GPIO_WritePin(BMI160_CS_PORT, BMI160_CS_PIN, GPIO_PIN_RESET)
#define BMI160_CS_HIGH() HAL_GPIO_WritePin(BMI160_CS_PORT, BMI160_CS_PIN, GPIO_PIN_SET)

// thanh ghi đọc dữ liệu cảm biến 
// thanh ghi đọc dữ liệu con quay hồi chuyển 
#define BMI_GYR_X_LSB 0x0C
#define BMI_GYR_X_MSB 0x0D
#define BMI_GYR_Y_LSB 0x0E
#define BMI_GYR_Y_MSB 0x0F  
#define BMI_GYR_Z_LSB 0x10
#define BMI_GYR_Z_MSB 0x11
// thanh ghi đọc dữ liệu gia tốc kế
#define BMI_ACC_X_LSB 0x12
#define BMI_ACC_X_MSB 0x13
#define BMI_ACC_Y_LSB 0x14
#define BMI_ACC_Y_MSB 0x15
#define BMI_ACC_Z_LSB 0x16
#define BMI_ACC_Z_MSB 0x17
// thanh ghi đọc dữ liệu nhiệt độ
#define BMI_TEMP_LSB 0x20
#define BMI_TEMP_MSB 0x21


// thanh ghi cấu hình cảm biến
// thanh ghi cấu hình con quay hồi chuyển
#define BMI_GYR_CONFIG 0x42
#define BMI_GYR_RANGE 0x43
// thanh ghi cấu hình gia tốc kế
#define BMI_ACC_CONFIG 0x40
#define BMI_ACC_RANGE 0x41


// thanh ghi điều khiển cảm biến
#define BMI_CMD 0x7E


// thanh ghi kiểm tra và trạng thái cảm biến
#define BMI_CHIP_ID 0x00
#define BMI_PMU_STATUS 0x03
#define BMI_STATUS 0x1B
#define BMI_ERROR 0x02


// -------------------các giá trị mặc định và phạm vi cấu hình-------------------
// các giá trị cấu hình chế độ hoạt động và tần số lấy mẫu của gia tốc kế
#define BMI160_ACC_US_DISABLE     0x00
#define BMI160_ACC_US_ENABLE      0x80
#define BMI160_ACC_BWP_NORMAL     0x10  // acc_bwp = 010
#define BMI160_ACC_BWP_OSR4       0x00  // acc_bwp = 000
#define BMI160_ACC_BWP_OSR2       0x08  // acc_bwp = 001
#define BMI160_ACC_ODR_1600HZ     0x08  // 1600Hz
#define BMI160_ACC_ODR_800HZ      0x09  // 800Hz
#define BMI160_ACC_ODR_400HZ      0x0A  // 400Hz
#define BMI160_ACC_ODR_200HZ      0x0B  // 200Hz
#define BMI160_ACC_ODR_100HZ      0x0C  // 100Hz
#define BMI160_ACC_ODR_50HZ       0x0D  // 50Hz
#define BMI160_ACC_ODR_25HZ       0x0E  // 25Hz

// các giá trị cấu hình phạm vi đo lường của gia tốc kế
#define BMI160_ACC_RANGE_2G       0x03
#define BMI160_ACC_RANGE_4G       0x05
#define BMI160_ACC_RANGE_8G       0x08
#define BMI160_ACC_RANGE_16G      0x0C

// các giá trị cấu hình con quay hồi chuyển
#define BMI160_GYR_BWP_NORMAL     0x02  // gyr_bwp = 010
#define BMI160_GYR_BWP_OSR4       0x00
#define BMI160_GYR_BWP_OSR2       0x01
#define BMI160_GYR_ODR_3200HZ     0x07  // 3200Hz
#define BMI160_GYR_ODR_1600HZ     0x08  // 1600Hz
#define BMI160_GYR_ODR_800HZ      0x09  // 800Hz
#define BMI160_GYR_ODR_400HZ      0x0A  // 400Hz
#define BMI160_GYR_ODR_200HZ      0x0B  // 200Hz
#define BMI160_GYR_ODR_100HZ      0x0C  // 100Hz
#define BMI160_GYR_ODR_50HZ       0x0D  // 50Hz
#define BMI160_GYR_ODR_25HZ       0x0E  // 25Hz

// các giá trị cấu hình phạm vi đo lường của con quay hồi chuyển
#define BMI160_GYR_RANGE_2000DPS  0x00
#define BMI160_GYR_RANGE_1000DPS  0x01
#define BMI160_GYR_RANGE_500DPS   0x02
#define BMI160_GYR_RANGE_250DPS   0x03
#define BMI160_GYR_RANGE_125DPS   0x04

// Các lệnh điều khiển cảm biến
#define BMI160_CMD_SOFTRESET      0xB6
#define BMI160_CMD_FIFO_FLUSH     0xB0
#define BMI160_CMD_START_FOC      0x03

// giá trị ID của cảm biến, dùng để kiểm tra kết nối và xác thực cảm biến khi đọc ID
#define BMI160_CHIP_ID_VALUE      0xD1  // theo datasheet

// các hệ số chuyển đổi từ giá trị thô đọc được sang đơn vị thực tế
#define BMI160_ACC_SENS_2G        16384  // LSB/g
#define BMI160_ACC_SENS_4G        8192   // LSB/g
#define BMI160_ACC_SENS_8G        4096   // LSB/g
#define BMI160_ACC_SENS_16G       2048   // LSB/g
#define BMI160_GYR_SENS_125DPS    262.4f // LSB/deg/s
#define BMI160_GYR_SENS_250DPS    131.2f // LSB/deg/s
#define BMI160_GYR_SENS_500DPS    65.6f  // LSB/deg/s
#define BMI160_GYR_SENS_1000DPS   32.8f  // LSB/deg/s
#define BMI160_GYR_SENS_2000DPS   16.4f  // LSB/deg/s


// cấu trúc dữ liệu để lưu trữ giá trị cảm biến
typedef struct {
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
    int16_t gyr_x;
    int16_t gyr_y;
    int16_t gyr_z;
    int16_t temp;
} BMI160_Data;

// cấu trúc dữ liệu để lưu trữ cấu hình cảm biến
typedef struct {
    uint8_t accel_range;  // Ví dụ: ±2g, ±4g, ±8g, ±16g
    uint8_t gyro_range;   // Ví dụ: ±125, ±250, ±500, ±1000, ±2000 dps
    uint8_t accel_odr;    // Ví dụ: 0.78Hz, 1.56Hz, 3.12Hz, 6.25Hz, 12.5Hz, 25Hz, 50Hz, 100Hz, 200Hz, 400Hz, 800Hz, 1600Hz
    uint8_t gyro_odr;     // Ví dụ: 0.78Hz, 1.56Hz, 3.12Hz, 6.25Hz, 12.5Hz, 25Hz, 50Hz, 100Hz, 200Hz, 400Hz, 800Hz, 1600Hz
    uint8_t accel_bwp;    // Ví dụ: Normal, OSR2, OSR4, OSR8, OSR16
    uint8_t gyro_bwp;     // Ví dụ: Normal, OSR2, OSR4, OSR8, OSR16
} BMI160_Config_t;

// cấu trúc dữ liệu để lưu trữ thông tin về cảm biến và cấu hình hiện tại của cảm biến
typedef struct{
    SPI_HandleTypeDef *hspi;       // Handle của bus SPI trên STM32 
    BMI160_Config_t config;        // Cấu hình hiện tại của cảm biến
    bool is_initialized;           // Trạng thái khởi tạo thành công hay chưa
} bmi_dev_t; 


// Hàm khởi tạo cảm biến (nhận struct quản lý, con trỏ SPI và cấu hình ban đầu)
HAL_StatusTypeDef BMI160_Init(bmi_dev_t *dev, SPI_HandleTypeDef *hspi, const BMI160_Config_t *config);

// Hàm đọc dữ liệu cảm biến, bao gồm gia tốc kế, con quay hồi chuyển và nhiệt độ
HAL_StatusTypeDef BMI160_ReadData(bmi_dev_t *dev, BMI160_Data *data);

// hàm cấu hình cảm biến bao gồm cấu hình con quay hồi chuyển và gia tốc kế
HAL_StatusTypeDef BMI160_Config(bmi_dev_t *dev);

// hàm kiểm tra kết nối cảm biến, trả về true nếu cảm biến phản hồi đúng ID, false nếu không kết nối được
HAL_StatusTypeDef BMI160_CheckConnection(bmi_dev_t *dev);

// hàm đọc ID cảm biến, trả về mã ID hoặc lỗi nếu không đọc được
HAL_StatusTypeDef BMI160_ReadID(bmi_dev_t *dev, uint8_t *id);

// hàm đọc trạng thái hoạt động của cảm biến, trả về mã trạng thái hoặc 0 nếu không có lỗi
HAL_StatusTypeDef BMI160_ReadStatus(bmi_dev_t *dev, uint8_t *status);

// hàm đọc lỗi từ cảm biến, trả về mã lỗi nếu có hoặc 0 nếu không có lỗi
HAL_StatusTypeDef BMI160_ReadError(bmi_dev_t *dev, uint8_t *error);

// hàm gửi lệnh điều khiển đến cảm biến, ví dụ để reset cảm biến hoặc chuyển đổi chế độ hoạt động
HAL_StatusTypeDef BMI160_SendCommand(bmi_dev_t *dev, uint8_t cmd);

// hàm đọc dữ liệu cảm biến, bao gồm gia tốc kế, con quay hồi chuyển và nhiệt độ
HAL_StatusTypeDef BMI160_ReadAxisData(bmi_dev_t *dev, int16_t *acc_x, int16_t *acc_y, int16_t *acc_z, int16_t *gyr_x, int16_t *gyr_y, int16_t *gyr_z);

// hàm đọc dữ liệu nhiệt độ từ cảm biến
HAL_StatusTypeDef BMI160_ReadTemperature(bmi_dev_t *dev, int16_t *temp);

// hàm cấu hình con quay hồi chuyển với các tham số cấu hình và phạm vi đo lường mong muốn
HAL_StatusTypeDef BMI160_ConfigGyro(bmi_dev_t *dev, uint8_t config, uint8_t range);

// hàm cấu hình Accelerometer(gia tốc kế) với các tham số cấu hình và phạm vi đo lường mong muốn
HAL_StatusTypeDef BMI160_ConfigAccel(bmi_dev_t *dev, uint8_t config, uint8_t range);

#endif // BMI160_H
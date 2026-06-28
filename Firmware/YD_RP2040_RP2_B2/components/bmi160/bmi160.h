#ifndef BMI160_H
#define BMI160_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// --------------------các định nghĩa về cấu hình I2C -------------------

// Cấu hình bộ ngoại vi I2C mặc định
#define BMI160_I2C_PORT  i2c0  // GPIO 20 và 21 thuộc bộ i2c0 trên RP2040
#define BMI160_PIN_SDA   20    // SDA  → GPIO 20
#define BMI160_PIN_SCL   21    // SCL  → GPIO 21
#define BMI160_I2C_ADDR  0x68  // Địa chỉ I2C khi SA0 nối GND

// --------------------các định nghĩa về địa chỉ thanh ghi -------------------

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
#define BMI_GYR_RANGE  0x43
// thanh ghi cấu hình gia tốc kế
#define BMI_ACC_CONFIG 0x40
#define BMI_ACC_RANGE  0x41

// thanh ghi điều khiển cảm biến
#define BMI_CMD 0x7E

// thanh ghi kiểm tra và trạng thái cảm biến
#define BMI_CHIP_ID    0x00
#define BMI_PMU_STATUS 0x03
#define BMI_STATUS     0x1B

// thanh ghi cấu hình FIFO
#define BMI_FIFO_DOWNSAMPLE 0x45
#define BMI_FIFO_CONFIG_0   0x46
#define BMI_FIFO_CONFIG_1   0x47

// thanh ghi báo lỗi hệ thống 
#define BMI_ERR_REG 0x02 

// ---------------------------------------các giá trị mặc định và phạm vi cấu hình---------------------------------------------

// Cấu hình Accelerometer - gia tốc kế : địa chỉ : 0x40 
#define BMI_ACC_CONFIG_DEFAULT                      0x28 
#define BMI_ACC_CONFIG_LOW_POWER                    0x26 
#define BMI_ACC_CONFIG_HIGH_PERFORMANCE             0x29 
#define BMI_ACC_CONFIG_BALANCED                     0x48 
#define BMI_ACC_CONFIG_HIGH_PERFORMANCE_FILTERED    0x4A

// dải đo Accelerometer - gia tốc kế : địa chỉ : 0x41
#define BMI_ACC_RANGE_2G              0x03
#define BMI_ACC_RANGE_4G              0x05
#define BMI_ACC_RANGE_8G              0x08
#define BMI_ACC_RANGE_16G             0x0C

// Cấu hình con quay hồi chuyển - Gyroscope : địa chỉ : 0x42
#define BMI_GYR_CONFIG_DEFAULT                  0x28
#define BMI_GYR_CONFIG_LOW_POWER                0x26
#define BMI_GYR_CONFIG_HIGH_PERFORMANCE         0x29
#define BMI_GYR_CONFIG_SMOOTH                   0x48
#define BMI_GYR_CONFIG_HIGH_SPEED_FILTERED      0x4A

// dải đo con quay hồi chuyển - Gyroscope : địa chỉ : 0x43
#define BMI_GYR_RANGE_2000DPS                   0x00
#define BMI_GYR_RANGE_1000DPS                   0x01
#define BMI_GYR_RANGE_500DPS                    0x02
#define BMI_GYR_RANGE_250DPS                    0x03
#define BMI_GYR_RANGE_125DPS                    0x04

// PMU_STATUS - trạng thái hoạt động của cảm biến : địa chỉ : 0x03
#define BMI_PMU_ACC_SUSPEND         0x00 
#define BMI_PMU_ACC_NORMAL          0x10 
#define BMI_PMU_ACC_LOWPOWER        0x20 
#define BMI_PMU_GYR_SUSPEND         0x00 
#define BMI_PMU_GYR_NORMAL          0x04 
#define BMI_PMU_GYR_FASTSTART       0x0C 

// FIFO_DOWNSAMPLE - cấu hình tốc độ đọc dữ liệu từ FIFO : địa chỉ : 0x45
#define BMI_FIFO_DOWNS_DEFAULT                  0x88
#define BMI_FIFO_DOWNS_FILTERED_NO_DIV          0x00
#define BMI_FIFO_DOWNS_FILTERED_DIV_4           0x22
#define BMI_FIFO_DOWNS_FILTERED_DIV_16          0x44

// FIFO_CONFIG - cấu hình chế độ hoạt động của FIFO1/FIFO2 : địa chỉ : 0x46 / 0x47
#define BMI_FIFO_WATERMARK_10_FRAMES            0x1E 
#define BMI_FIFO_WATERMARK_DEFAULT              0x80 
#define BMI_FIFO_DATA_SEL_ACC_ONLY              0x48
#define BMI_FIFO_DATA_SEL_GYR_ONLY              0x88
#define BMI_FIFO_DATA_SEL_ACC_GYR               0xC8
#define BMI_FIFO_DATA_SEL_ACC_GYR_TIME          0xCA

// CMD - lệnh điều khiển cảm biến : địa chỉ : 0x7E
#define BMI_CMD_ACC_NET_NORMAL                  0x11 
#define BMI_CMD_ACC_NET_LOWPOWER                0x12 
#define BMI_CMD_GYR_NET_NORMAL                  0x15 
#define BMI_CMD_GYR_NET_FASTSTART               0x17 
#define BMI_CMD_MAG_NET_NORMAL                  0x19 
#define BMI_DELAY_ACC_PMU_MS                    4    
#define BMI_DELAY_GYRO_PMU_MS                   80   
#define BMI_DELAY_MAG_PMU_MS                    1    
#define BMI_CMD_SOFT_RESET                      0xB6 
#define BMI_CMD_FIFO_FLUSH                      0xB0 
#define BMI_CMD_INT_RESET                       0xB1 
#define BMI_CMD_START_FOC                       0x03 
#define BMI_CMD_PROG_NVM                        0xA0 

// CHIP_ID - mã ID của cảm biến : địa chỉ : 0x00
#define BMI_CHIPID_VALUE                        0xD1

// ERROR - mã lỗi của cảm biến : địa chỉ : 0x02
#define BMI_ERR_DROP_CMD_MASK                   0x40 
#define BMI_ERR_CODE_MASK                       0x1E 
#define BMI_ERR_FATAL_MASK                      0x01 
#define BMI_ERR_CODE_NONE                       0x00 
#define BMI_ERR_CODE_LP_INT_PREFILT             0x06 
#define BMI_ERR_CODE_FIFO_ODR_MISMATCH          0x0C 
#define BMI_ERR_CODE_LP_PREFILT                 0x0E 

// --------------------các cấu trúc dữ liệu và hàm API-------------------

typedef enum {
    BMI_OK    = 0,
    BMI_ERROR = 1,
} BMI_Status;

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
    uint8_t accel_range;  
    uint8_t gyro_range;   
    uint8_t accel_odr;    
    uint8_t gyro_odr;     
} BMI160_Config_t;

// cấu trúc dữ liệu để lưu trữ thông tin về cảm biến và cấu hình hiện tại
typedef struct {
    i2c_inst_t *handle_i2c;     // Con trỏ tới I2C instance (i2c0 hoặc i2c1)
    uint8_t     i2c_addr;       // Địa chỉ I2C của thiết bị (0x68 hoặc 0x69)
    BMI160_Config_t config;     // Cấu hình hiện tại của cảm biến
    bool         is_initialized; // Trạng thái khởi tạo thành công hay chưa
} bmi_dev_t;

// Hàm khởi tạo cảm biến (nhận struct quản lý, con trỏ I2C, địa chỉ thiết bị và cấu hình ban đầu)
BMI_Status BMI160_Init(bmi_dev_t *dev, i2c_inst_t *handle_i2c, uint8_t i2c_addr, const BMI160_Config_t *config);

// Hàm đọc dữ liệu cảm biến, bao gồm gia tốc kế, con quay hồi chuyển
BMI_Status BMI160_ReadData(bmi_dev_t *dev, BMI160_Data *data);

// hàm cấu hình cảm biến bao gồm cấu hình con quay hồi chuyển và gia tốc kế
BMI_Status BMI160_Config(bmi_dev_t *dev);

// hàm kiểm tra kết nối cảm biến, trả về BMI_OK nếu cảm biến phản hồi đúng ID
BMI_Status BMI160_CheckConnection(bmi_dev_t *dev);

// hàm đọc ID cảm biến, trả về mã ID hoặc lỗi nếu không đọc được
BMI_Status BMI160_ReadID(bmi_dev_t *dev, uint8_t *id);

// hàm đọc trạng thái hoạt động của cảm biến
BMI_Status BMI160_ReadStatus(bmi_dev_t *dev, uint8_t *status);

// hàm đọc lỗi từ cảm biến, trả về mã lỗi nếu có hoặc 0 nếu không có lỗi
BMI_Status BMI160_ReadError(bmi_dev_t *dev, uint8_t *error);

// hàm gửi lệnh điều khiển đến cảm biến, ví dụ để reset cảm biến hoặc chuyển đổi chế độ hoạt động
BMI_Status BMI160_SendCommand(bmi_dev_t *dev, uint8_t cmd);

// hàm đọc dữ liệu nhiệt độ từ cảm biến
BMI_Status BMI160_ReadTemperature(bmi_dev_t *dev, int16_t *temp);

// hàm cấu hình con quay hồi chuyển với các tham số cấu hình và phạm vi đo lường mong muốn
BMI_Status BMI160_ConfigGyro(bmi_dev_t *dev, uint8_t config, uint8_t range);

// hàm cấu hình Accelerometer(gia tốc kế) với các tham số cấu hình và phạm vi đo lường mong muốn
BMI_Status BMI160_ConfigAccel(bmi_dev_t *dev, uint8_t config, uint8_t range);

#ifdef __cplusplus
}
#endif

#endif // BMI160_H
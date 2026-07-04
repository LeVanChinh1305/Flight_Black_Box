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
#define BMI_CMD 0x7E                            // thanh ghi gửi lệnh điều khiển cảm biến

// thanh ghi kiểm tra và trạng thái cảm biến
#define BMI_CHIP_ID    0x00                     // thanh ghi đọc mã ID của cảm biến
#define BMI_PMU_STATUS 0x03                     // thanh ghi đọc trạng thái hoạt động của cảm biến
#define BMI_STATUS     0x1B                     // thanh ghi đọc trạng thái cảm biến

// thanh ghi cấu hình FIFO
#define BMI_FIFO_DOWNSAMPLE 0x45
#define BMI_FIFO_CONFIG_0   0x46
#define BMI_FIFO_CONFIG_1   0x47

// thanh ghi đọc dữ liệu FIFO
#define BMI_FIFO_DATA 0x24                       // thanh ghi đọc dữ liệu FIFO
#define BMI_FIFO_LENGTH_0 0x22                  // thanh ghi đọc độ dài dữ liệu FIFO (LSB) - byte thấp
#define BMI_FIFO_LENGTH_1 0x23                  // thanh ghi đọc độ dài dữ liệu FIFO (MSB) - byte cao


// thanh ghi báo lỗi hệ thống 
#define BMI_ERR_REG 0x02                        // thanh ghi đọc mã lỗi của cảm biến








// ---------------------------------------các giá trị mặc định và phạm vi cấu hình---------------------------------------------

// Cấu hình Accelerometer - gia tốc kế : địa chỉ : 0x40 
#define BMI_ACC_CONFIG_DEFAULT                      0x28 // chế độ hoạt động bình thường, tần số lấy mẫu 100Hz, bộ lọc LPF 50Hz
#define BMI_ACC_CONFIG_LOW_POWER                    0x26 // chế độ hoạt động tiết kiệm năng lượng, tần số lấy mẫu 25Hz, bộ lọc LPF 12.5Hz
#define BMI_ACC_CONFIG_HIGH_PERFORMANCE             0x29 // chế độ hoạt động hiệu năng cao, tần số lấy mẫu 200Hz, bộ lọc LPF 100Hz
#define BMI_ACC_CONFIG_BALANCED                     0x48 // chế độ hoạt động cân bằng, tần số lấy mẫu 100Hz, bộ lọc LPF 50Hz
#define BMI_ACC_CONFIG_HIGH_PERFORMANCE_FILTERED    0x4A // chế độ hoạt động hiệu năng cao, tần số lấy mẫu 200Hz, bộ lọc LPF 100Hz, dữ liệu được lọc và giảm tốc độ lấy mẫu xuống 50Hz

// dải đo Accelerometer - gia tốc kế : địa chỉ : 0x41
#define BMI_ACC_RANGE_2G              0x03 // dải đo 2G
#define BMI_ACC_RANGE_4G              0x05 // dải đo 4G
#define BMI_ACC_RANGE_8G              0x08 // dải đo 8G
#define BMI_ACC_RANGE_16G             0x0C // dải đo 16G

// Cấu hình con quay hồi chuyển - Gyroscope : địa chỉ : 0x42
#define BMI_GYR_CONFIG_DEFAULT                  0x28 // chế độ hoạt động bình thường, tần số lấy mẫu 100Hz, bộ lọc LPF 50Hz
#define BMI_GYR_CONFIG_LOW_POWER                0x26 // chế độ hoạt động tiết kiệm năng lượng, tần số lấy mẫu 25Hz, bộ lọc LPF 12.5Hz
#define BMI_GYR_CONFIG_HIGH_PERFORMANCE         0x29 // chế độ hoạt động hiệu năng cao, tần số lấy mẫu 200Hz, bộ lọc LPF 100Hz
#define BMI_GYR_CONFIG_SMOOTH                   0x48 // chế độ hoạt động mượt mà, tần số lấy mẫu 100Hz, bộ lọc LPF 50Hz
#define BMI_GYR_CONFIG_HIGH_SPEED_FILTERED      0x4A // chế độ hoạt động hiệu năng cao, tần số lấy mẫu 200Hz, bộ lọc LPF 100Hz, dữ liệu được lọc và giảm tốc độ lấy mẫu xuống 50Hz

// dải đo con quay hồi chuyển - Gyroscope : địa chỉ : 0x43
#define BMI_GYR_RANGE_2000DPS                   0x00 // dải đo 2000 độ/giây
#define BMI_GYR_RANGE_1000DPS                   0x01 // dải đo 1000 độ/giây
#define BMI_GYR_RANGE_500DPS                    0x02 // dải đo 500 độ/giây
#define BMI_GYR_RANGE_250DPS                    0x03 // dải đo 250 độ/giây
#define BMI_GYR_RANGE_125DPS                    0x04 // dải đo 125 độ/giây

// PMU_STATUS - trạng thái hoạt động của cảm biến : địa chỉ : 0x03
#define BMI_PMU_ACC_SUSPEND         0x00 // chế độ tạm dừng hoạt động của gia tốc kế
#define BMI_PMU_ACC_NORMAL          0x10 // chế độ hoạt động bình thường của gia tốc kế
#define BMI_PMU_ACC_LOWPOWER        0x20 // chế độ tiết kiệm năng lượng của gia tốc kế
#define BMI_PMU_GYR_SUSPEND         0x00 // chế độ tạm dừng hoạt động của con quay hồi chuyển
#define BMI_PMU_GYR_NORMAL          0x04 // chế độ hoạt động bình thường của con quay hồi chuyển
#define BMI_PMU_GYR_FASTSTART       0x0C // chế độ khởi động nhanh của con quay hồi chuyển

// FIFO_DOWNSAMPLE - cấu hình tốc độ đọc dữ liệu từ FIFO : địa chỉ : 0x45
#define BMI_FIFO_DOWNS_DEFAULT                  0x88 // cấu hình mặc định, dữ liệu FIFO được đọc với tốc độ bình thường, không giảm tốc độ lấy mẫu
#define BMI_FIFO_DOWNS_FILTERED_NO_DIV          0x00 // 
#define BMI_FIFO_DOWNS_FILTERED_DIV_4           0x22 // 
#define BMI_FIFO_DOWNS_FILTERED_DIV_16          0x44 //

// FIFO_CONFIG - cấu hình chế độ hoạt động của FIFO1/FIFO2 : địa chỉ : 0x46 / 0x47
#define BMI_FIFO_WATERMARK_10_FRAMES            0x1E 
#define BMI_FIFO_WATERMARK_DEFAULT              0x80 
#define BMI_FIFO_DATA_SEL_ACC_ONLY              0x48
#define BMI_FIFO_DATA_SEL_GYR_ONLY              0x88
#define BMI_FIFO_DATA_SEL_ACC_GYR               0xC8
#define BMI_FIFO_DATA_SEL_ACC_GYR_TIME          0xCA

// FIFO_CONFIG[1] (0x47) - từng bit riêng lẻ, dùng khi build cấu hình động qua BMI160_FIFO_Config_t
#define BMI_FIFO_GYR_EN_MASK          0x80   // bật lưu dữ liệu gyro vào FIFO
#define BMI_FIFO_ACC_EN_MASK          0x40   // bật lưu dữ liệu accel vào FIFO
#define BMI_FIFO_MAG_EN_MASK          0x20   // bật lưu dữ liệu mag vào FIFO (chưa hỗ trợ trong driver này)
#define BMI_FIFO_HEADER_EN_MASK       0x10   // bật header mode (1) / headerless mode (0)
#define BMI_FIFO_TAG_INT1_EN_MASK     0x08   // gắn tag ngắt INT1 vào frame (chỉ header mode)
#define BMI_FIFO_TAG_INT2_EN_MASK     0x04   // gắn tag ngắt INT2 vào frame (chỉ header mode)
#define BMI_FIFO_TIME_EN_MASK         0x02   // trả về sensortime frame khi đọc quá dữ liệu hợp lệ

// Kích thước FIFO vật lý của BMI160 (byte) và kích thước 1 frame regular acc+gyr (byte)
#define BMI_FIFO_BUFFER_SIZE          1024
#define BMI_FIFO_FRAME_LEN_ACC_GYR    12   // 6 byte gyro + 6 byte accel (không header)
#define BMI_FIFO_FRAME_LEN_ACC        6
#define BMI_FIFO_FRAME_LEN_GYR        6

// FIFO frame header (header mode) - các mask để giải mã byte header theo datasheet mục 2.5.1.3
#define BMI_FIFO_HDR_MODE_MASK        0xC0   // bit 7:6 - fh_mode
#define BMI_FIFO_HDR_MODE_REGULAR     0x80   // 0b10 << 6 : frame dữ liệu cảm biến
#define BMI_FIFO_HDR_MODE_CONTROL     0x40   // 0b01 << 6 : frame điều khiển (skip / sensortime / config)
#define BMI_FIFO_HDR_PARM_MASK        0x3C   // bit 5:2 - fh_parm<3:0>
#define BMI_FIFO_HDR_PARM_SHIFT       2
#define BMI_FIFO_HDR_EXT_MASK         0x03   // bit 1:0 - fh_ext<1:0> (tag ngắt)

#define BMI_FIFO_HDR_INVALID          0x80   // header 0x80 = frame chưa khởi tạo / hết dữ liệu hợp lệ (fh_parm=0000, mode=10)

// fh_parm<2:0> cho regular frame (bit trong phần đã shift, tức parm & 0x0F)
#define BMI_FIFO_PARM_ACC_BIT         0x01
#define BMI_FIFO_PARM_GYR_BIT         0x02
#define BMI_FIFO_PARM_MAG_BIT         0x04

// fh_parm cho control frame (opcode)
#define BMI_FIFO_CTRL_SKIP_FRAME      0x00
#define BMI_FIFO_CTRL_SENSORTIME      0x01
#define BMI_FIFO_CTRL_INPUT_CONFIG    0x02


// CMD - lệnh điều khiển cảm biến : địa chỉ : 0x7E
#define BMI_CMD_ACC_NET_NORMAL                  0x11   // chế độ hoạt động bình thường của gia tốc kế
#define BMI_CMD_ACC_NET_LOWPOWER                0x12   // chế độ tiết kiệm năng lượng của gia tốc kế
#define BMI_CMD_GYR_NET_NORMAL                  0x15   // chế độ hoạt động bình thường của con quay hồi chuyển
#define BMI_CMD_GYR_NET_FASTSTART               0x17   // chế độ khởi động nhanh của con quay hồi chuyển
#define BMI_CMD_MAG_NET_NORMAL                  0x19   // chế độ hoạt động bình thường của từ kế
#define BMI_DELAY_ACC_PMU_MS                    4      // thời gian chờ để gia tốc kế khởi động và sẵn sàng đọc dữ liệu
#define BMI_DELAY_GYRO_PMU_MS                   80     // thời gian chờ để con quay hồi chuyển khởi động và sẵn sàng đọc dữ liệu
#define BMI_DELAY_MAG_PMU_MS                    1      // thời gian chờ để từ kế khởi động và sẵn sàng đọc dữ liệu
#define BMI_CMD_SOFT_RESET                      0xB6   // lệnh reset cảm biến
#define BMI_CMD_FIFO_FLUSH                      0xB0   // lệnh xóa dữ liệu FIFO
#define BMI_CMD_INT_RESET                       0xB1   // lệnh reset ngắt
#define BMI_CMD_START_FOC                       0x03   // lệnh bắt đầu hiệu chuẩn con quay hồi chuyển
#define BMI_CMD_PROG_NVM                        0xA0   // lệnh lập trình bộ nhớ không bay hơi (NVM)

// CHIP_ID - mã ID của cảm biến : địa chỉ : 0x00
#define BMI_CHIPID_VALUE                        0xD1

// ERROR - mã lỗi của cảm biến : địa chỉ : 0x02
#define BMI_ERR_DROP_CMD_MASK                   0x40 // lỗi bỏ qua lệnh
#define BMI_ERR_CODE_MASK                       0x1E // lỗi mã
#define BMI_ERR_FATAL_MASK                      0x01 // lỗi nghiêm trọng
#define BMI_ERR_CODE_NONE                       0x00 // không có lỗi
#define BMI_ERR_CODE_LP_INT_PREFILT             0x06 // lỗi bộ lọc LPF trong chế độ tiết kiệm năng lượng
#define BMI_ERR_CODE_FIFO_ODR_MISMATCH          0x0C // lỗi không khớp tốc độ lấy mẫu FIFO
#define BMI_ERR_CODE_LP_PREFILT                 0x0E // lỗi bộ lọc LPF trong chế độ tiết kiệm năng lượng
















// -------------------------------------các cấu trúc dữ liệu -------------------------------------------

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

// cấu trúc dữ liệu để lưu trữ cấu hình FIFO
typedef struct {
    bool acc_en; // bật lưu dữ liệu accel vào FIFO
    bool gyr_en; // bật lưu dữ liệu gyro vào FIFO
    bool header_en; // true: header mode (khuyến nghị, linh hoạt) | false: headerless mode
    bool time_en; // true: trả về sensortime frame khi đọc quá dữ liệu hợp lệ | false: không trả về sensortime frame
    uint8_t watermark; // ngưỡng cảnh báo dữ liệu FIFO (0-255), khi đạt ngưỡng này sẽ phát sinh ngắt FIFO
} BMI160_FIFO_Config_t;

// 1 frame dữ liệu FIFO (header mode) - cấu trúc dữ liệu để lưu trữ dữ liệu FIFO đã giải mã
typedef struct{
    bool has_acc; // true nếu frame có dữ liệu accel, false nếu không có
    bool has_gyr; // true nếu frame có dữ liệu gyro, false nếu không có
    int16_t acc_x; // dữ liệu accel X (LSB)
    int16_t acc_y; // dữ liệu accel Y (LSB)
    int16_t acc_z; // dữ liệu accel Z (LSB)
    int16_t gyr_x; // dữ liệu gyro X (LSB)
    int16_t gyr_y; // dữ liệu gyro Y (LSB)
    int16_t gyr_z; // dữ liệu gyro Z (LSB)
} BMI160_FIFO_Frame_t;


// 1 frame dữ liệu FIFO (headerless mode) - cấu trúc dữ liệu để lưu trữ dữ liệu FIFO đã giải mã
typedef struct{
    uint16_t frame_count; // số frame dữ liệu hợp lệ đã giải mã được từ FIFO
    uint16_t bytes_read; // số byte dữ liệu đã đọc từ FIFO_DATA
    uint16_t skipped_frames; // số frame dữ liệu bị bỏ qua do không hợp lệ hoặc không có header
    bool overflow_detected; // true nếu phát hiện tràn FIFO, false nếu không
} BMI160_FIFO_Result_t;

// cấu trúc dữ liệu để lưu trữ thông tin về cảm biến và cấu hình hiện tại
typedef struct {
    i2c_inst_t *handle_i2c;     // Con trỏ tới I2C instance (i2c0 hoặc i2c1)
    uint8_t     i2c_addr;       // Địa chỉ I2C của thiết bị (0x68 hoặc 0x69)
    BMI160_Config_t config;     // Cấu hình hiện tại của cảm biến
    BMI160_FIFO_Config_t fifo_config; // Cấu hình FIFO hiện tại của cảm biến
    bool         is_initialized; // Trạng thái khởi tạo thành công hay chưa
} bmi_dev_t;

 

// ------------------------------------------hàm API--------------------------------------------

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

// hàm cấu hình FIFO với các tham số cấu hình mong muốn: chọn sensor nào được lưu, header/headerless mode, watermark, sensortime.
BMI_Status BMI160_FIFO_Config(bmi_dev_t *dev, const BMI160_FIFO_Config_t *fifo_config);

// hàm đọc số byte hiện có trong FIFO, trả về số byte dữ liệu hợp lệ hiện có trong FIFO
BMI_Status BMI160_FIFO_GetLength(bmi_dev_t *dev, uint16_t *length);

// đọc length byte dữ liệu từ FIFO_DATA, lưu vào buffer do người dùng cung cấp
BMI_Status BMI160_FIFO_Read(bmi_dev_t *dev, uint8_t *buffer, uint16_t length);

// xóa toàn bộ dữ liệu hiện có trong FIFO, reset FIFO về trạng thái rỗng
BMI_Status BMI160_FIFO_Flush(bmi_dev_t *dev);

// giải mã (parse) một buffer byte thô đọc từ FIFO thành mảng BMI160_FIFO_Frame_t.
BMI160_FIFO_Result_t BMI160_FIFO_ParseFrames(bmi_dev_t *dev, const uint8_t *buffer, uint16_t buf_len,
                                              BMI160_FIFO_Frame_t *frames, uint16_t max_frames);

                                
// hàm tiện ích: đọc toàn bộ FIFO hiện có và giải mã luôn trong 1 lần gọi (gộp GetLength + Read + ParseFrames).
BMI_Status BMI160_FIFO_ReadAndParse(bmi_dev_t *dev, uint8_t *scratch_buf, uint16_t scratch_buf_size,
                                     BMI160_FIFO_Frame_t *frames, uint16_t max_frames,
                                     BMI160_FIFO_Result_t *result);

#ifdef __cplusplus
}
#endif

#endif // BMI160_H
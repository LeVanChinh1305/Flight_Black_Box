#ifndef BMI160_H
#define BMI160_H

#include "stm32f1xx_hal.h"
#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

// --------------------các định nghĩa về địa chỉ thanh ghi -------------------

// địa chỉ của cảm biến trên bus SPI

#define BMI160_SPI_READ_MASK  0x80
#define BMI160_SPI_WRITE_MASK 0x7F


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


// thanh ghi cấu hình FIFO
#define BMI_FIFO_DOWNSAMPLE 0x45
#define BMI_FIFO_CONFIG_0 0x46
#define BMI_FIFO_CONFIG_1 0x47

// thanh ghi báo lỗi hệ thống 
#define BMI_ERR_REG 0x02 



// ---------------------------------------các giá trị mặc định và phạm vi cấu hình---------------------------------------------

// Cấu hình Accelerometer - gia tốc kế : địa chỉ : 0x40 
// Thiết lập tốc độ dữ liệu đầu ra, băng thông và chế độ đọc của cảm biến gia tốc.
// Định nghĩa: [Bit 7: acc_us] [Bit 6:4: acc_bwp] [Bit 3:0: acc_odr]
// giá trị mặc định nhà sản xuất khuyến nghị :         ODR = 100Hz, Bandwidth =  Normal Mode, Undersampling = Off => 0010 1000 =>  0x28 
#define BMI_ACC_CONFIG_DEFAULT                      0x28 
// tiết kiệm pin :                                     ODR = 25Hz, Bandwidth = Normal Mode, Undersampling = Off => 0010 0110 => 0x26 => dùng trong ứng dụng cần tiết kiệm pin, không yêu cầu tốc độ đọc dữ liệu cao hoặc phản hồi nhanh(vd: đếm bước chân, thiết bị đeo tay theo dõi sức khỏe, cảm biến chuyển động trong nhà thông minh)
#define BMI_ACC_CONFIG_LOW_POWER                    0x26 
// Tốc độ cao / Phản hồi nhanh :                       ODR = 200Hz, Bandwidth = Normal Mode, Undersampling = Off => 0010 1001 => 0x29 => dùng trong ứng dụng cần tốc độ đọc dữ liệu cao hoặc phản hồi nhanh, không quan tâm đến mức tiêu thụ năng lượng(vd: robot, drone, thiết bị chơi game)
#define BMI_ACC_CONFIG_HIGH_PERFORMANCE             0x29 
// Ứng dụng phổ thông :                                ODR = 100Hz, Bandwidth = Cic Filter, Undersampling = Off =>  0100 1000 => 0x48 => Đo góc nghiêng, điều khiển mượt mà :
#define BMI_ACC_CONFIG_BALANCED                     0x48 
// Tốc độ cao + Lọc nhiễu mạnh :                       ODR = 400Hz, Bandwidth = Cic Filter, Undersampling = Off => 0100 1010 => 0x4A => dùng trong ứng dụng cần tốc độ đọc dữ liệu cao hoặc phản hồi nhanh, đồng thời cần giảm thiểu nhiễu trong dữ liệu cảm biến(vd: robot, drone, thiết bị chơi game trong môi trường nhiều nhiễu)
#define BMI_ACC_CONFIG_HIGH_PERFORMANCE_FILTERED    0x4A


// dải đo Accelerometer - gia tốc kế : địa chỉ : 0x41
// Định nghĩa: [Bit 7:4: reserved] [Bit 3:0: acc_range]
// lựa chọn phạm vi gia tốc g của gia tốc kế
// Lưu ý: Sau khi đổi Range, nên đọc ngay thanh ghi DATA (0x04-0x17) để xóa dữ liệu cũ.
// Khoảng đo +/-2g (Mặc định nhà sản xuất): 0000 0011 = 0x03 -> Độ phân giải cao nhất (nhạy nhất). 
// -> Ứng dụng: Đo góc nghiêng tĩnh, gimbal, thiết bị đeo tay theo dõi sức khỏe, đếm bước chân.
#define BMI_ACC_RANGE_2G              0x03
// Khoảng đo +/-4g: 0000 0101 = 0x05
// -> Ứng dụng: Điều khiển drone, tay cầm chơi game chuyển động, robot di chuyển tốc độ trung bình.
#define BMI_ACC_RANGE_4G              0x05
// Khoảng đo +/-8g: 0000 1000 = 0x08
// -> Ứng dụng: Các chuyển động nhanh, nhảy cao, xe máy/ô tô di chuyển trên địa hình gồ ghề.
#define BMI_ACC_RANGE_8G              0x08
// Khoảng đo +/-16g: 0000 1100 = 0x0C -> Độ phân giải thấp nhất nhưng không bị kịch thang đo khi có lực tác động lớn.
// -> Ứng dụng: Hộp đen ghi nhận va chạm mạnh (Crash/Impact Recorder), túi khí ô tô, tên lửa mô hình.
#define BMI_ACC_RANGE_16G             0x0C


// Cấu hình con quay hồi chuyển - Gyroscope : địa chỉ : 0x42
// Định nghĩa: [Bit 7: Reserved = 0] [Bit 6:4: gyr_bwp] [Bit 3:0: gyr_odr]
// Khuyến nghị từ NSX: Bandwidth = Normal, ODR = 100Hz -> Nhị phân: 0010 1000 = 0x28
// -> Ứng dụng: Cân bằng tiêu chuẩn, đo góc nghiêng/xoay cơ bản cho hệ thống chậm.
#define BMI_GYR_CONFIG_DEFAULT                  0x28
// Chế độ tiết kiệm pin tối đa: Bandwidth = Normal, ODR = 25Hz -> Nhị phân: 0010 0110 = 0x26
// -> Ứng dụng: Nhận diện cử chỉ xoay cổ tay cơ bản, thiết bị đeo đeo tay thông minh (Smartwatch).
#define BMI_GYR_CONFIG_LOW_POWER                0x26
// Tốc độ cao / Phản hồi nhanh: Bandwidth = Normal, ODR = 200Hz -> Nhị phân: 0010 1001 = 0x29
// -> Ứng dụng: Hộp đen ghi nhận quỹ đạo chuyển động, chuột bay (Air mouse), robot tự hành tầm trung.
#define BMI_GYR_CONFIG_HIGH_PERFORMANCE         0x29
// Ứng dụng phổ thông nâng cao: Bandwidth = Cic Filter (0b100), ODR = 100Hz -> Nhị phân: 0100 1000 = 0x48
// -> Ứng dụng: Đo góc mượt mà kết hợp với bộ lọc Kalman/Complementary, Gimbal chống rung.
#define BMI_GYR_CONFIG_SMOOTH                   0x48
// Tốc độ cao + Lọc nhiễu mạnh: Bandwidth = Cic Filter (0b100), ODR = 400Hz (0b1010) -> Nhị phân: 0100 1010 = 0x4A
// -> Ứng dụng: Hệ thống điều khiển bay cho Drone, UAV hoặc Robot cân bằng động hoạt động môi trường rung lớn.
#define BMI_GYR_CONFIG_HIGH_SPEED_FILTERED      0x4A



// dải đo con quay hồi chuyển - Gyroscope : địa chỉ : 0x43
// Định nghĩa: [Bit 7:3: Reserved = 0] [Bit 2:0: gyr_range]
// Lưu ý : Sau khi đổi Range, nên đọc ngay thanh ghi DATA (0x04-0x17) để xóa dữ liệu cũ.
// Khoảng đo +/-2000 dps (Mặc định): 0000 0000 = 0x00 -> Tầm đo rộng nhất, khó bị kịch thang đo khi quay mạnh. Độ nhạy: 16.4 LSB/(deg/s)
// -> Ứng dụng: Thiết bị bay thể thao (Drone), tay cầm chơi game hành động mạnh, ghi nhận va đập xoay.
#define BMI_GYR_RANGE_2000DPS                   0x00
// Khoảng đo +/-1000 dps: 0000 0001 = 0x01 -> Độ nhạy: 32.8 LSB/(deg/s)
// -> Ứng dụng: Robot di chuyển tốc độ cao, chuột bay quay quét nhanh.
#define BMI_GYR_RANGE_1000DPS                   0x01
// Khoảng đo +/-500 dps: 0000 0010 = 0x02 -> Độ nhạy: 65.6 LSB/(deg/s)
// -> Ứng dụng: Hệ thống robot tự hành (AGV/AMR), tracking chuyển động người thông thường.
#define BMI_GYR_RANGE_500DPS                    0x02
// Khoảng đo +/-250 dps: 0000 0011 = 0x03 -> Độ nhạy: 131.2 LSB/(deg/s)
// -> Ứng dụng: Đo góc nghiêng chuyển động mượt, Gimbal cầm tay, thiết bị đeo theo dõi cử chỉ tay.
#define BMI_GYR_RANGE_250DPS                    0x03
// Khoảng đo +/-125 dps: 0000 0100 = 0x04 -> Độ phân giải cao nhất (nhạy nhất với các góc xoay cực nhỏ). Độ nhạy: 262.4 LSB/(deg/s)
// -> Ứng dụng: Các ứng dụng cần đo tư thế/góc tĩnh siêu chính xác, kính thiên văn, bệ đỡ camera quét chậm.
#define BMI_GYR_RANGE_125DPS                    0x04



// PMU_STATUS - trạng thái hoạt động của cảm biến : địa chỉ : 0x03
// Đọc trạng thái nguồn hiện tại (Power Mode Status) của cả 3 khối cảm biến bên trong chip
// dùng khi cần kiểm tra xem cảm biến đã sẵn sàng hoạt động chưa sau khi khởi tạo hoặc gửi lệnh thay đổi chế độ hoạt động.
// định nghĩa : [Bit 7:6: reserved] [Bit 5:4: acc_pmu_status] [Bit 3:2: gyr_pmu_status] [Bit 1:0: mag_pmu_status]
// --- Khối Gia tốc (Accelerometer) ---
#define BMI_ACC_PMU_MASK                        0x30 // Mặt nạ Bit [5:4] (0011 0000)
#define BMI_ACC_PMU_SUSPEND                     0x00 // 0b00 << 4
#define BMI_ACC_PMU_NORMAL                      0x10 // 0b01 << 4
#define BMI_ACC_PMU_LOW_POWER                   0x20 // 0b10 << 4
// --- Khối Con quay hồi chuyển (Gyroscope) ---
#define BMI_GYR_PMU_MASK                        0x0C // Mặt nạ Bit [3:2] (0000 1100)
#define BMI_GYR_PMU_SUSPEND                     0x00 // 0b00 << 2
#define BMI_GYR_PMU_NORMAL                      0x04 // 0b01 << 2
#define BMI_GYR_PMU_FAST_STARTUP                0x0C // 0b11 << 2
// --- Khối Từ trường (Magnetometer) ---
#define BMI_MAG_PMU_MASK                        0x03 // Mặt nạ Bit [1:0] (0000 0011)
#define BMI_MAG_PMU_SUSPEND                     0x00 // 0b00
#define BMI_MAG_PMU_NORMAL                      0x01 // 0b01
#define BMI_MAG_PMU_LOW_POWER                   0x02 // 0b10


// STATUS - trạng thái hoạt động của cảm biến : địa chỉ : 0x1B
// chế độ là Chỉ đọc
// các cờ trạng thái (status flags) hiện tại của cảm biến: bao gồm cờ dữ liệu mới sẵn sàng, cờ lỗi, cờ tràn FIFO, v.v.
// định nghĩa : 
    // bit 7: drdy_acc: cờ dữ liệu mới của gia tốc kế đã sẵn sàng để đọc (1 = dữ liệu mới, 0 = không có dữ liệu mới)
    // bit 6: drdy_gyr: cờ dữ liệu mới của con quay hồi chuyển đã sẵn sàng để đọc (1 = dữ liệu mới, 0 = không có dữ liệu mới)
    // bit 5: drdy_mag: cờ dữ liệu mới của từ trường kế đã sẵn sàng để đọc (1 = dữ liệu mới, 0 = không có dữ liệu mới)
    // bit 4: nvm_rdy: cờ bộ nhớ không bay hơi (NVM) đã sẵn sàng (1 = NVM sẵn sàng, 0 = NVM không sẵn sàng). NVM là bộ nhớ không bay hơi bên trong chip dùng để lưu trữ cấu hình và hiệu chỉnh. Cờ này cho biết liệu NVM đã sẵn sàng để đọc/ghi hay chưa.
    // bit 3: foc_rdy: cờ tự hiệu chỉnh (Factory Offset Compensation) đã hoàn thành (1 = tự hiệu chỉnh hoàn thành, 0 = tự hiệu chỉnh chưa hoàn thành). Cờ này cho biết liệu quá trình tự hiệu chỉnh ban đầu của cảm biến đã hoàn tất hay chưa, điều này quan trọng để đảm bảo dữ liệu cảm biến chính xác.
    // bit 2: mag_man_op: cờ hoạt động thủ công của từ trường kế đang diễn ra (1 = đang hoạt động thủ công, 0 = không hoạt động thủ công). Cờ này cho biết liệu cảm biến từ trường kế có đang thực hiện một hoạt động thủ công nào đó (ví dụ: hiệu chỉnh, đo đặc biệt) hay không.
    // bit 1: gyr_self_test_ok: rạng thái kiểm tra lỗi phần cứng của Gyro (Gyroscope self-test status). Nếu bit này là 1, nghĩa là kiểm tra tự động của con quay hồi chuyển đã thành công và cảm biến đang hoạt động bình thường. Nếu bit này là 0, có thể có vấn đề với phần cứng của con quay hồi chuyển hoặc quá trình tự kiểm tra đã thất bại.
    // bit 0: reserved (0) 
#define BMI_STATUS_DRDY_ACC_MASK                0x80 // Bit 7: 1000 0000 (Data Ready Accel)
#define BMI_STATUS_DRDY_GYR_MASK                0x40 // Bit 6: 0100 0000 (Data Ready Gyro)
#define BMI_STATUS_DRDY_MAG_MASK                0x20 // Bit 5: 0010 0000 (Data Ready Magnet)
#define BMI_STATUS_NVM_RDY_MASK                 0x10 // Bit 4: 0001 0000
#define BMI_STATUS_FOC_RDY_MASK                 0x08 // Bit 3: 0000 1000
#define BMI_STATUS_GYR_SELF_TEST_MASK           0x02 // Bit 1: 0000 0010 



// FIFO_DOWNSAMPLE - cấu hình tốc độ đọc dữ liệu từ FIFO : địa chỉ : 0x45
// // Định nghĩa: [ 7 : acc_filt][ 6:4 acc_downs] | [ 3: gyr_filt][ 2:0 gyr_downs]
// Mặc định nhà sản xuất: Dữ liệu thô (Pre-filtered), không hạ mẫu (Ratio = 1)
// -> Nhị phân: 1000 1000 = 0x88
#define BMI_FIFO_DOWNS_DEFAULT                  0x88
// Dữ liệu đã qua lọc (Filtered), không hạ mẫu (Ratio = 1)
// -> Nhị phân: 0000 0000 = 0x00 -> Phù hợp khi cần dữ liệu FIFO mượt, ít nhiễu.
#define BMI_FIFO_DOWNS_FILTERED_NO_DIV          0x00
// Dữ liệu đã qua lọc (Filtered), hạ mẫu 1/4 cho cả Acc và Gyro
// -> Nhị phân: 0010 0010 = 0x22 -> Tiết kiệm không gian FIFO gấp 4 lần.
#define BMI_FIFO_DOWNS_FILTERED_DIV_4           0x22
// Dữ liệu đã qua lọc (Filtered), hạ mẫu mạnh 1/16 cho cả Acc và Gyro
// -> Nhị phân: 0100 0100 = 0x44 -> Dùng khi ODR đặt rất cao (ví dụ 1600Hz) nhưng chỉ muốn lưu vào FIFO ở mức 100Hz.
#define BMI_FIFO_DOWNS_FILTERED_DIV_16          0x44


// FIFO_CONFIG - cấu hình chế độ hoạt động của FIFO1/FIFO2 : địa chỉ : 0x46 / 0x47
// 0x46: Chuyên dùng để cấu hình mức Watermark (ngưỡng báo ngắt đầy bộ đệm).
// fifo_water_mark<7:0>
// Chức năng: Định nghĩa một ngưỡng (Watermark level). Khi số lượng byte dữ liệu lưu trữ hiện tại trong bộ đệm FIFO vượt quá ngưỡng này,
// cảm biến sẽ tự động kích hoạt một tín hiệu ngắt phần cứng (FIFO Watermark Interrupt) để báo cho MCU biết đã đến lúc cần vào đọc dữ liệu ra.
// Đơn vị tính: 4 byte. (Ví dụ: Nếu ghi giá trị 10 vào thanh ghi này, ngưỡng ngắt thực tế sẽ là 10 x 4 = 40 byte). Reset Value: 0b10000000 = 0x80 (Tương đương với $128 \times 4 = 512$ byte).
#define BMI_FIFO_WATERMARK_10_FRAMES            0x1E // Ngưỡng 10 khung dữ liệu (120 byte) - phù hợp cho ứng dụng cần phản hồi nhanh, đọc dữ liệu thường xuyên.
#define BMI_FIFO_WATERMARK_DEFAULT              0x80 // Mặc định: 512 byte

// 0x47: Chuyên dùng để lựa chọn loại dữ liệu nào sẽ được phép ghi vào FIFO và định dạng của chúng.
// Chỉ lưu Gia tốc kế (Accel), chế độ có Header (Mặc định gọn nhẹ cho đo bước/va chạm)-> Nhị phân: 0100 1000 = 0x48
#define BMI_FIFO_DATA_SEL_ACC_ONLY              0x48
// Chỉ lưu Con quay hồi chuyển (Gyro), chế độ có Header -> Nhị phân: 1000 1000 = 0x88
#define BMI_FIFO_DATA_SEL_GYR_ONLY              0x88
// Lưu cả Accel và Gyro, chế độ có Header (Thường dùng nhất cho các bộ ghi hành trình 6-DOF) -> Nhị phân: 1100 1000 = 0xC8
#define BMI_FIFO_DATA_SEL_ACC_GYR               0xC8
// Lưu cả Accel, Gyro + Kèm luôn thời gian thực của cảm biến (Sensortime) -> Nhị phân: 1100 1010 = 0xCA
#define BMI_FIFO_DATA_SEL_ACC_GYR_TIME          0xCA



// CMD - lệnh điều khiển cảm biến : địa chỉ : 0x7E
// chỉ nhận các mã lệnh (Command Codes) dạng Hex cố định để kích hoạt các hành vi phần cứng
// --- Mã lệnh Quản lý Nguồn (PMU Modes) ---
#define BMI_CMD_ACC_NET_NORMAL                  0x11 // Bật Gia tốc kế sang Normal Mode
#define BMI_CMD_ACC_NET_LOWPOWER                0x12 // Bật Gia tốc kế sang Low Power Mode
#define BMI_CMD_GYR_NET_NORMAL                  0x15 // Bật Gyro sang Normal Mode
#define BMI_CMD_GYR_NET_FASTSTART               0x17 // Bật Gyro sang Fast Start-Up Mode
#define BMI_CMD_MAG_NET_NORMAL                  0x19 // Bật Magnet sang Normal Mode
// --- Thời gian delay tối đa bắt buộc (Max Execution Time) ---
#define BMI_DELAY_ACC_PMU_MS                    4    // Thực tế tối đa 3.8ms
#define BMI_DELAY_GYRO_PMU_MS                   80   // Thực tế tối đa 80ms
#define BMI_DELAY_MAG_PMU_MS                    1    // Thực tế tối đa 0.5ms
// --- Mã lệnh Điều khiển Hệ thống ---
#define BMI_CMD_SOFT_RESET                      0xB6 // Reset toàn bộ chip phần cứng
#define BMI_CMD_FIFO_FLUSH                      0xB0 // Xóa sạch bộ đệm dữ liệu FIFO
#define BMI_CMD_INT_RESET                       0xB1 // Reset bộ xử lý ngắt và chân ngắt
#define BMI_CMD_START_FOC                       0x03 // Chạy hiệu chuẩn Fast Offset
#define BMI_CMD_PROG_NVM                        0xA0 // Ghi cấu hình vào bộ nhớ NVM



// CHIP_ID - mã ID của cảm biến : địa chỉ : 0x00
#define BMI_CHIPID_VALUE                        0xD1



// ERROR - mã lỗi của cảm biến : địa chỉ : 0x02
//Mặt nạ kiểm tra (Bit Masks) 
#define BMI_ERR_DROP_CMD_MASK                   0x40 // Bit 6: Cờ báo nuốt lệnh
#define BMI_ERR_CODE_MASK                       0x1E // Bit [4:1]: Mặt nạ mã lỗi (0001 1110)
#define BMI_ERR_FATAL_MASK                      0x01 // Bit 0: Cờ báo lỗi phần cứng
// Tra cứu Mã lỗi hệ thống (Error Codes) 
#define BMI_ERR_CODE_NONE                       0x00 // 0b0000 << 1
#define BMI_ERR_CODE_LP_INT_PREFILT             0x06 // 0b0011 << 1 (Lỗi bộ lọc mạch ngắt)
#define BMI_ERR_CODE_FIFO_ODR_MISMATCH          0x0C // 0b0110 << 1 (Lỗi lệch ODR trong Headerless FIFO)
#define BMI_ERR_CODE_LP_PREFILT                 0x0E // 0b0111 << 1 (Lỗi bộ lọc chế độ Low-Power)



// --------------------các cấu trúc dữ liệu và hàm API-------------------

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
    SPI_HandleTypeDef *handle_spi;       // Handle của bus SPI trên STM32 
    BMI160_Config_t config;        // Cấu hình hiện tại của cảm biến
    bool is_initialized;           // Trạng thái khởi tạo thành công hay chưa
    GPIO_TypeDef      *cs_port;    // Port của chân Chip Select (Ví dụ: GPIOA, GPIOB)
    uint16_t          cs_pin;      // Pin của chân Chip Select (Ví dụ: GPIO_PIN_3)
} bmi_dev_t; 


// Hàm khởi tạo cảm biến (nhận struct quản lý, con trỏ SPI và cấu hình ban đầu)
HAL_StatusTypeDef BMI160_Init(bmi_dev_t *dev, SPI_HandleTypeDef *handle_spi, GPIO_TypeDef *cs_port, uint16_t cs_pin, const BMI160_Config_t *config);

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

// hàm đọc dữ liệu nhiệt độ từ cảm biến
HAL_StatusTypeDef BMI160_ReadTemperature(bmi_dev_t *dev, int16_t *temp);

// hàm cấu hình con quay hồi chuyển với các tham số cấu hình và phạm vi đo lường mong muốn
HAL_StatusTypeDef BMI160_ConfigGyro(bmi_dev_t *dev, uint8_t config, uint8_t range);

// hàm cấu hình Accelerometer(gia tốc kế) với các tham số cấu hình và phạm vi đo lường mong muốn
HAL_StatusTypeDef BMI160_ConfigAccel(bmi_dev_t *dev, uint8_t config, uint8_t range);


#ifdef __cplusplus
}
#endif 
#endif // BMI160_H
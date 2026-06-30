// #include <stdio.h>
// #include "pico/stdlib.h"
// #include "components/bmi160/bmi160.h"

// int main() {
//     stdio_init_all();
//     while (!stdio_usb_connected()) sleep_ms(100);
//     sleep_ms(500);

//     printf("\n========== BMI160 DATA READ : 6 HZ ==========\n\n");

//     // ── Cấu hình ban đầu cho cảm biến ──
//     BMI160_Config_t bmi_config = {
//         .accel_range = BMI_ACC_RANGE_2G,
//         .gyro_range  = BMI_GYR_RANGE_2000DPS,
//         .accel_odr   = BMI_ACC_CONFIG_DEFAULT,
//         .gyro_odr    = BMI_GYR_CONFIG_DEFAULT
//     };

//     // ── Khai báo cấu trúc quản lý thiết bị ──
//     bmi_dev_t bmi_device;

//     // ── Gọi hàm khởi tạo từ driver ──
//     if (BMI160_Init(&bmi_device, BMI160_I2C_PORT, BMI160_I2C_ADDR, &bmi_config) == BMI_OK) {
//         printf("\n>>> KET NOI OK! BMI160 san sang hoat dong.\n");
//     } else {
//         printf("\n>>> LOI: Khoi tao BMI160 that bai!\n");
//         printf("    Vui long kiem tra lai day noi SDA/SCL, cap nguon, va dam bao chan CS da keo len 3V3.\n");
//         while (1) sleep_ms(1000); // Dừng chương trình nếu lỗi khởi tạo
//     }

//     // ── Khai báo struct lưu trữ dữ liệu cảm biến ──
//     BMI160_Data sensor_data;
//     uint32_t count = 0;

//     // Tính toán thời gian trễ: 1000ms / 6 lần = ~166.67ms (lấy xấp xỉ 167ms)
//     const uint32_t sample_interval_ms = 167;

//     printf("\n>>> Bat dau doc du lieu (6 lan/giay):\n\n");

//     while (1) {
//         // Gọi hàm đọc 12 byte dữ liệu thô từ thanh ghi (gồm cả Accel và Gyro)
//         if (BMI160_ReadData(&bmi_device, &sensor_data) == BMI_OK) {
//             printf("[%lu] ACC: X=%6d, Y=%6d, Z=%6d | GYRO: X=%6d, Y=%6d, Z=%6d\n", 
//                    ++count,
//                    sensor_data.acc_x, sensor_data.acc_y, sensor_data.acc_z,
//                    sensor_data.gyr_x, sensor_data.gyr_y, sensor_data.gyr_z);
//         } else {
//             printf("[%lu] Giao tiep I2C gap LOI khi doc du lieu!\n", ++count);
//         }

//         // Tạo độ trễ để đạt tốc độ ~6Hz
//         sleep_ms(sample_interval_ms);
//     }
// }







//-----------------------------------------------------test neo6m------------------------------------------------------------------








// #include <stdio.h>
// #include "pico/stdlib.h"
// #include "components/neo6m/neo6m.h"

// int main() {
//     // Khởi tạo luồng giao tiếp với máy tính (USB Serial Monitor)
//     stdio_init_all();
//     while (!stdio_usb_connected()) sleep_ms(100);
//     sleep_ms(500);

//     printf("\n========== NEO-6M GPS TESTING SYSTEM ==========\n\n");

//     neo6m_dev_t gps_dev;
//     neo6m_data_t gps_data;
    
//     gps_data.is_valid = false;
//     gps_data.satellites = 0;

//     // Tiến hành kết nối UART phần cứng
//     printf("[INIT] Dang thiet lap ket noi UART cho Neo-6M...\n");
//     if (NEO6M_Init(&gps_dev, NEO6M_UART_PORT, NEO6M_PIN_TX, NEO6M_PIN_RX, NEO6M_BAUDRATE) != NEO6M_OK) {
//         printf("[FATAL] Khoi tao module GPS that bai! Dung chuong trinh.\n");
//         while (1) sleep_ms(1000);
//     }
    
//     printf("[OK] Module da san sang. Dang quet tin hieu...\n");
//     printf("------------------------------------------------------------------------\n");

//     uint32_t message_count = 0;

//     while (1) {
//         // Liên tục kiểm tra luồng UART nhận câu lệnh NMEA mới
//         NEO6M_Status status = NEO6M_Update(&gps_dev, &gps_data);
        
//         if (status == NEO6M_OK) {
//             message_count++;
            
//             // Log chuỗi thô từ module phát ra
//             printf("[%lu] Raw NMEA: %s\n", message_count, gps_dev.nmea_buf);

//             // Xử lý hiển thị trực quan nếu module đã "Khóa vị trí" (Fix vệ tinh thành công)
//             if (gps_data.is_valid) {
//                 // Tự động chuyển đổi từ khung giờ UTC sang giờ Việt Nam (GMT+7)
//                 int local_hour = gps_data.hour + 7;
//                 int local_day = gps_data.day;
//                 int local_month = gps_data.month;
//                 int local_year = gps_data.year;

//                 if (local_hour >= 24) {
//                     local_hour -= 24;
//                     local_day += 1; 
//                 }

//                 printf("    >> [STATUS: FIX OK]\n");
//                 printf("       + Thoi gian (VN): %02d:%02d:%02d  | Ngay: %02d/%02d/%04d\n", 
//                        local_hour, gps_data.minute, gps_data.second, local_day, local_month, local_year);
//                 printf("       + Toa do         : Lat = %.6f, Lon = %.6f\n", 
//                        gps_data.latitude, gps_data.longitude);
//                 printf("       + Thong so ve tinh: Bat duoc = %d ve tinh | Do chinh xac HDOP = %.2f\n", 
//                        gps_data.satellites, gps_data.hdop);
//                 printf("       + Van toc & Cao do: Toc do = %.2f km/h | Do cao = %.1fm\n", 
//                        gps_data.speed_kmh, gps_data.altitude_m);
//             } else {
//                 printf("    >> [STATUS: NO FIX] Dang quet tim ve tinh (Ve tinh bat duoc: %d)\n", gps_data.satellites);
//             }
//             printf("------------------------------------------------------------------------\n");
//         } else {
//             // Delay cực ngắn để giải phóng CPU khi UART rảnh hoặc gặp timeout tạm thời giữa các luồng truyền tải
//             sleep_us(100);
//         }
//     }
// }













//---------------------------------------test tft--------------------------------------------------------------


#include <stdio.h>
#include "pico/stdlib.h"
#include "components/tft/tft.h"

int main() {
    // Khởi tạo luồng giao tiếp với máy tính (USB Serial Monitor)
    stdio_init_all();
    while (!stdio_usb_connected()) sleep_ms(100);
    sleep_ms(500);

    printf("\n========== TFT 2.8 inch (ILI9341) - HELLO TEST ==========\n\n");

    // ── Khai báo cấu trúc quản lý thiết bị ──
    tft_dev_t tft_dev;

    // ── Gọi hàm khởi tạo từ driver ──
    if (TFT_Init(&tft_dev, TFT_SPI_PORT) == TFT_OK) {
        printf(">>> KET NOI OK! Man hinh TFT san sang hoat dong.\n");
    } else {
        printf(">>> LOI: Khoi tao man hinh TFT that bai!\n");
        printf("    Vui long kiem tra lai day noi SPI (SDO/SDI/SCL/CS/RST/DC), cap nguon 3V3.\n");
        while (1) sleep_ms(1000); // Dừng chương trình nếu lỗi khởi tạo
    }

    // ── Xoá nền và in chữ "Hello" lên giữa màn hình ──
    TFT_FillScreen(&tft_dev, TFT_COLOR_WHITE);
    TFT_DrawString(&tft_dev, 40, 100, "Hello", TFT_COLOR_BLACK, TFT_COLOR_WHITE, 4);

    printf(">>> Da in 'Hello' len man hinh.\n");

    while (1) {
        tight_loop_contents();
    }
}
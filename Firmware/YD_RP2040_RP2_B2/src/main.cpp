#include <stdio.h>
#include "pico/stdlib.h"
#include "components/bmi160/bmi160.h"

int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) sleep_ms(100);
    sleep_ms(500);

    printf("\n========== BMI160 I2C DEBUG : 2 ==========\n\n");

    // ── cấu hình ban đầu cho cảm biến ──
    BMI160_Config_t bmi_config = {
        .accel_range = BMI_ACC_RANGE_2G,
        .gyro_range  = BMI_GYR_RANGE_2000DPS,
        .accel_odr   = BMI_ACC_CONFIG_DEFAULT,
        .gyro_odr    = BMI_GYR_CONFIG_DEFAULT
    };

    // ── khai báo cấu trúc quản lý thiết bị ──
    bmi_dev_t bmi_device;

    // ── gọi hàm khởi tạo từ driver ──
    // Toàn bộ công việc chi tiết (Khởi tạo bộ I2C0, cấu hình chân GPIO 20 & 21, Reset, Kiểm tra ID, Bật nguồn) đều được thực hiện bên trong Driver
    if (BMI160_Init(&bmi_device, BMI160_I2C_PORT, BMI160_I2C_ADDR, &bmi_config) == BMI_OK) {
        printf("\n>>> KET NOI OK! BMI160 phan hoi dung qua giao tiep I2C.\n");
    } else {
        printf("\n>>> LOI: Khoi tao BMI160 that bai!\n");
        printf("    Vui long kiem tra lai day noi SDA/SCL, cap nguon, va dam bao chan CS da keo len 3V3.\n");
    }

    // ── đọc liên tục để kiểm tra ổn định ──
    printf("\n[6] Doc CHIP_ID lien tuc moi 1 giay qua I2C:\n");
    uint32_t count = 0;
    uint8_t id = 0;
    while (1) {
        if (BMI160_ReadID(&bmi_device, &id) == BMI_OK) {
            printf("    lan %lu: 0x%02X %s\n", ++count, id,
                   id == BMI_CHIPID_VALUE ? "OK" : "FAIL");
        } else {
            printf("    lan %lu: Giao tiep I2C gap LOI!\n", ++count);
        }
        sleep_ms(1000);
    }
}
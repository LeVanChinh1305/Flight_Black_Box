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









// #include <stdio.h>
// #include "pico/stdlib.h"
// #include "components/tft/tft.h"

// int main() {
//     // Khởi tạo luồng giao tiếp với máy tính (USB Serial Monitor)
//     stdio_init_all();
//     while (!stdio_usb_connected()) sleep_ms(100);
//     sleep_ms(500);

//     printf("\n========== TFT 2.8 inch (ILI9341) - HELLO TEST ==========\n\n");

//     // ── Khai báo cấu trúc quản lý thiết bị ──
//     tft_dev_t tft_dev;

//     // ── Gọi hàm khởi tạo từ driver ──
//     if (TFT_Init(&tft_dev, TFT_SPI_PORT) == TFT_OK) {
//         printf(">>> KET NOI OK! Man hinh TFT san sang hoat dong.\n");
//     } else {
//         printf(">>> LOI: Khoi tao man hinh TFT that bai!\n");
//         printf("    Vui long kiem tra lai day noi SPI (SDO/SDI/SCL/CS/RST/DC), cap nguon 3V3.\n");
//         while (1) sleep_ms(1000); // Dừng chương trình nếu lỗi khởi tạo
//     }

//     // ── Xoá nền và in chữ "Hello" lên giữa màn hình ──
//     TFT_FillScreen(&tft_dev, TFT_COLOR_WHITE);
//     TFT_DrawString(&tft_dev, 40, 100, "Hello", TFT_COLOR_BLACK, TFT_COLOR_WHITE, 4);

//     printf(">>> Da in 'Hello' len man hinh.\n");

//     while (1) {
//         tight_loop_contents();
//     }
// }









//------------------------------------test xpt ----------------------------------------------







// #include <stdio.h>
// #include "pico/stdlib.h"
// #include "components/tft/tft.h"
// #include "components/xpt2046/xpt2046.h"

// // ====================== HÀM VẼ MÀN HÌNH ======================
// void draw_screen(bool show_hello, tft_dev_t *tft) {
//     TFT_FillScreen(tft, TFT_COLOR_WHITE);
    
//     // Chữ chính
//     if (show_hello) {
//         TFT_DrawString(tft, 45, 80, "Hello", TFT_COLOR_BLACK, TFT_COLOR_WHITE, 5);
//     } else {
//         TFT_DrawString(tft, 85, 75, "HI", TFT_COLOR_BLUE, TFT_COLOR_WHITE, 8);
//     }

//     // Nút bấm ĐỎ
//     TFT_FillRect(tft, 60, 200, 120, 60, TFT_COLOR_RED);

//     // Chữ trên nút
//     TFT_DrawString(tft, 78, 217, "TOUCH", TFT_COLOR_WHITE, TFT_COLOR_RED, 3);
//     TFT_DrawString(tft, 93, 242, "ME", TFT_COLOR_WHITE, TFT_COLOR_RED, 2);
// }

// // ====================== MAIN ======================
// int main() {
//     stdio_init_all();
//     while (!stdio_usb_connected()) sleep_ms(100);
//     sleep_ms(500);

//     printf("\n========== TFT + XPT2046 TOUCH (SPI0) ==========\n\n");

//     tft_dev_t tft_dev;
//     xpt2046_dev_t touch_dev;

//     // ====================== KHỞI TẠO TFT (SPI1) ======================
//     if (TFT_Init(&tft_dev, TFT_SPI_PORT) != TFT_OK) {
//         printf(">>> LOI: Khoi tao TFT that bai!\n");
//         while(1) sleep_ms(1000);
//     }

//     // ====================== KHỞI TẠO TOUCH (SPI0) ======================
//     if (XPT2046_Init(&touch_dev, spi0) != XPT_OK) {
//         printf(">>> LOI: Khoi tao XPT2046 that bai!\n");
//         while(1) sleep_ms(1000);
//     }

//     bool show_hello = true;
//     uint32_t last_touch_time = 0;

//     draw_screen(show_hello, &tft_dev);

//     printf(">>> San sang! Cham vao nut DO de chuyen chu.\n");
//     printf(">>> Dang theo doi IRQ va touch...\n\n");

//         while (1) {
//         XPT2046_Update(&touch_dev);

//         // Debug IRQ mỗi 300ms
//         static uint32_t last_debug = 0;
//         uint32_t now = to_ms_since_boot(get_absolute_time());

//         if (now - last_debug > 300) {
//             last_debug = now;
//             printf("IRQ=%d | is_touched=%d\n", 
//                    gpio_get(XPT2046_PIN_IRQ), 
//                    touch_dev.is_touched);
//         }

//         if (XPT2046_IsTouched(&touch_dev)) {
//             if (now - last_touch_time > 250) {   // debounce
//                 printf(">>> TOUCH DETECTED! Raw(%d,%d) -> Screen(%d,%d)\n",
//                        touch_dev.x_raw, touch_dev.y_raw, touch_dev.x, touch_dev.y);

//                 // Kiểm tra vùng nút đỏ
//                 if (touch_dev.x >= 50 && touch_dev.x <= 190 && 
//                     touch_dev.y >= 190 && touch_dev.y <= 270) {
                    
//                     show_hello = !show_hello;
//                     draw_screen(show_hello, &tft_dev);
                    
//                     printf(">>> DA NHAN NUT! Hien thi: %s\n", show_hello ? "Hello" : "HI");
//                 }
//                 last_touch_time = now;
//             }
//         }
//         sleep_ms(10);
//     }
// }













//----------------------------------------- test 4 moduele trên ----------------------------------------------------














// // =====================================================================================
// //  FLIGHT BLACK BOX - MAIN
// //  Hệ thống đọc cảm biến BMI160 (IMU) + NEO-6M (GPS), hiển thị trên TFT 240x320 (ILI9341)
// //  với 2 trang dữ liệu, chuyển trang bằng menu cảm ứng (XPT2046) ở đáy màn hình.

// //  Nguyên tắc vẽ màn hình:
// //   - Khi vào 1 trang: vẽ NHÃN (label) + khung tĩnh đúng 1 lần duy nhất.
// //   - Mỗi vòng lặp: chỉ tính giá trị mới -> so sánh với giá trị cũ đã hiển thị
// //     -> NẾU khác mới xoá vùng nhỏ (FillRect) và vẽ lại (DrawString) giá trị đó.
// //     -> NẾU giống thì giữ nguyên, không vẽ lại (tránh nháy màn hình / tốn thời gian SPI).
// // =====================================================================================

// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
// #include "pico/stdlib.h"

// #include "components/bmi160/bmi160.h"
// #include "components/neo6m/neo6m.h"
// #include "components/tft/tft.h"
// #include "components/xpt2046/xpt2046.h"

// // ============================== CẤU HÌNH GIAO DIỆN ==============================

// #define MENU_HEIGHT      48        // tăng chiều cao để vùng chạm rộng rãi, dễ bấm hơn
// #define MENU_Y            (TFT_HEIGHT - MENU_HEIGHT)     // y bắt đầu thanh menu
// #define MENU_BTN_W        (TFT_WIDTH / 2)                // mỗi nút chiếm nửa chiều rộng -> 2 nút phủ kín full màn hình ngang

// #define COLOR_BG          TFT_COLOR_WHITE
// #define COLOR_TITLE_BG    TFT_COLOR_BLUE
// #define COLOR_TITLE_FG    TFT_COLOR_WHITE
// #define COLOR_LABEL       TFT_COLOR_BLACK
// #define COLOR_VALUE       TFT_COLOR_BLUE
// #define COLOR_VALUE_BAD   TFT_COLOR_RED
// #define COLOR_MENU_OFF_BG TFT_COLOR_BLACK
// #define COLOR_MENU_OFF_FG TFT_COLOR_WHITE
// #define COLOR_MENU_ON_BG  TFT_COLOR_GREEN
// #define COLOR_MENU_ON_FG  TFT_COLOR_BLACK

// typedef enum {
//     PAGE_BMI160 = 0,
//     PAGE_GPS    = 1,
//     PAGE_COUNT  = 2,
// } app_page_t;

// // Một "ô dữ liệu" gồm: vị trí vẽ giá trị, độ rộng vùng (để xoá đúng phần cũ),
// // và chuỗi giá trị đã hiển thị lần gần nhất (dùng để so sánh, tránh vẽ lại thừa).
// typedef struct {
//     int16_t  x, y;
//     uint16_t w, h;
//     uint8_t  size;
//     char     last_text[24];   // "" nghĩa là chưa từng vẽ -> bắt buộc vẽ lần đầu
// } ui_field_t;

// // ============================== BIẾN TOÀN CỤC PHẦN CỨNG ==============================

// static tft_dev_t      g_tft;
// static xpt2046_dev_t  g_touch;
// static bmi_dev_t      g_bmi;
// static neo6m_dev_t    g_gps_dev;
// static neo6m_data_t   g_gps_data;

// static app_page_t g_current_page  = PAGE_BMI160;
// static app_page_t g_drawn_page    = (app_page_t)0xFF;   // ép vẽ tĩnh lần đầu

// // ----- Các ô dữ liệu trang BMI160 -----
// enum {
//     F_ACC_X, F_ACC_Y, F_ACC_Z,
//     F_GYR_X, F_GYR_Y, F_GYR_Z,
//     F_TEMP,
//     F_STATUS_BMI,
//     F_BMI_COUNT
// };
// static ui_field_t g_bmi_field[F_BMI_COUNT];

// // ----- Các ô dữ liệu trang GPS -----
// enum {
//     F_FIX, F_SAT, F_HDOP,
//     F_LAT, F_LON, F_ALT,
//     F_SPD_KMH, F_SPD_KNOT, F_COURSE,
//     F_TIME_UTC, F_DATE,
//     F_GPS_COUNT
// };
// static ui_field_t g_gps_field[F_GPS_COUNT];

// // ============================== HÀM TIỆN ÍCH VẼ ==============================

// // Vẽ 1 nhãn cố định (label), gọi 1 lần khi dựng trang.
// static void draw_label(int16_t x, int16_t y, const char *text, uint8_t size) {
//     TFT_DrawString(&g_tft, x, y, text, COLOR_LABEL, COLOR_BG, size);
// }

// // Khởi tạo 1 field: lưu toạ độ/kích thước vùng giá trị, đặt last_text rỗng
// // để vòng lặp đầu tiên chắc chắn vẽ giá trị khởi tạo.
// static void field_init(ui_field_t *f, int16_t x, int16_t y, uint16_t w, uint8_t size) {
//     f->x = x;
//     f->y = y;
//     f->w = w;
//     f->h = (uint8_t)(8 * size); // chiều cao 1 dòng chữ ứng với size font 5x7 (8 = 7 + dãn dòng)
//     f->size = size;
//     f->last_text[0] = '\0';
// }

// // Cập nhật giá trị của 1 field: chỉ vẽ lại nếu nội dung mới khác nội dung đã vẽ trước đó.
// static void field_update(ui_field_t *f, const char *new_text, uint16_t color) {
//     if (strncmp(f->last_text, new_text, sizeof(f->last_text)) == 0) {
//         return; // không đổi -> bỏ qua, không tải lại
//     }
//     // Xoá vùng giá trị cũ rồi vẽ giá trị mới
//     TFT_FillRect(&g_tft, f->x, f->y, f->w, f->h, COLOR_BG);
//     TFT_DrawString(&g_tft, f->x, f->y, new_text, color, COLOR_BG, f->size);
//     strncpy(f->last_text, new_text, sizeof(f->last_text) - 1);
//     f->last_text[sizeof(f->last_text) - 1] = '\0';
// }

// // Buộc 1 field vẽ lại ở vòng lặp kế tiếp (dùng khi vừa dựng lại trang)
// static void field_force_redraw(ui_field_t *f) {
//     f->last_text[0] = '\0';
// }

// // ============================== THANH MENU DƯỚI ĐÁY ==============================

// static void draw_menu_bar(app_page_t active) {
//     const uint8_t  txt_size = 3;                 // chữ to hơn cho dễ đọc/dễ nhắm khi bấm
//     const uint16_t txt_w    = 6 * txt_size;       // bề rộng ước lượng 1 ký tự (font 5x7 + khoảng cách)
//     const int16_t  txt_y    = MENU_Y + (MENU_HEIGHT - 8 * txt_size) / 2; // canh giữa theo chiều dọc nút

//     // Nút trang 1: BMI160 (full chiều rộng nửa trái, full chiều cao thanh menu)
//     uint16_t bg1 = (active == PAGE_BMI160) ? COLOR_MENU_ON_BG : COLOR_MENU_OFF_BG;
//     uint16_t fg1 = (active == PAGE_BMI160) ? COLOR_MENU_ON_FG : COLOR_MENU_OFF_FG;
//     TFT_FillRect(&g_tft, 0, MENU_Y, MENU_BTN_W, MENU_HEIGHT, bg1);
//     {
//         int16_t text_x = (MENU_BTN_W - (int16_t)(6 * txt_w)) / 2; // căn giữa chuỗi "BMI160" (6 ký tự)
//         if (text_x < 0) text_x = 4;
//         TFT_DrawString(&g_tft, text_x, txt_y, "BMI160", fg1, bg1, txt_size);
//     }

//     // Nút trang 2: GPS (full chiều rộng nửa phải, full chiều cao thanh menu)
//     uint16_t bg2 = (active == PAGE_GPS) ? COLOR_MENU_ON_BG : COLOR_MENU_OFF_BG;
//     uint16_t fg2 = (active == PAGE_GPS) ? COLOR_MENU_ON_FG : COLOR_MENU_OFF_FG;
//     uint16_t btn2_w = TFT_WIDTH - MENU_BTN_W;
//     TFT_FillRect(&g_tft, MENU_BTN_W, MENU_Y, btn2_w, MENU_HEIGHT, bg2);
//     {
//         int16_t text_x = MENU_BTN_W + (btn2_w - (int16_t)(3 * txt_w)) / 2; // căn giữa chuỗi "GPS" (3 ký tự)
//         TFT_DrawString(&g_tft, text_x, txt_y, "GPS", fg2, bg2, txt_size);
//     }

//     // Đường viền phân tách 2 nút
//     TFT_FillRect(&g_tft, MENU_BTN_W - 1, MENU_Y, 2, MENU_HEIGHT, TFT_COLOR_WHITE);
// }

// // Kiểm tra điểm chạm có rơi vào thanh menu không, trả về trang được chọn (hoặc -1)
// // Vùng chạm = TOÀN BỘ chiều rộng & chiều cao của mỗi nút (đã full màn hình theo chiều ngang),
// // không cần chạm trúng đúng vị trí chữ.
// static int hit_test_menu(int16_t x, int16_t y) {
//     if (y < MENU_Y) return -1;
//     if (x < 0 || x >= TFT_WIDTH) return -1;
//     return (x < MENU_BTN_W) ? PAGE_BMI160 : PAGE_GPS;
// }

// // ============================== DỰNG KHUNG TĨNH TRANG BMI160 ==============================

// static void build_static_bmi160_page(void) {
//     TFT_FillScreen(&g_tft, COLOR_BG);

//     // Thanh tiêu đề
//     TFT_FillRect(&g_tft, 0, 0, TFT_WIDTH, 22, COLOR_TITLE_BG);
//     TFT_DrawString(&g_tft, 8, 5, "BMI160 - IMU SENSOR", COLOR_TITLE_FG, COLOR_TITLE_BG, 2);

//     int16_t y = 34;
//     const int16_t row_h   = 22;
//     const int16_t label_x = 8;
//     const int16_t value_x = 140;
//     const uint16_t value_w = 95;

//     draw_label(label_x, y, "Trang thai:", 2); field_init(&g_bmi_field[F_STATUS_BMI], value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "Accel X:",   2); field_init(&g_bmi_field[F_ACC_X],     value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "Accel Y:",   2); field_init(&g_bmi_field[F_ACC_Y],     value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "Accel Z:",   2); field_init(&g_bmi_field[F_ACC_Z],     value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "Gyro X:",    2); field_init(&g_bmi_field[F_GYR_X],     value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "Gyro Y:",    2); field_init(&g_bmi_field[F_GYR_Y],     value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "Gyro Z:",    2); field_init(&g_bmi_field[F_GYR_Z],     value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "Nhiet do:",  2); field_init(&g_bmi_field[F_TEMP],      value_x, y, value_w, 2); y += row_h;

//     draw_menu_bar(PAGE_BMI160);
// }

// // ============================== DỰNG KHUNG TĨNH TRANG GPS ==============================

// static void build_static_gps_page(void) {
//     TFT_FillScreen(&g_tft, COLOR_BG);

//     TFT_FillRect(&g_tft, 0, 0, TFT_WIDTH, 22, COLOR_TITLE_BG);
//     TFT_DrawString(&g_tft, 8, 5, "NEO-6M - GPS MODULE", COLOR_TITLE_FG, COLOR_TITLE_BG, 2);

//     int16_t y = 34;
//     const int16_t row_h   = 22;
//     const int16_t label_x = 8;
//     const int16_t value_x = 110;
//     const uint16_t value_w = 125;

//     draw_label(label_x, y, "Fix:",        2); field_init(&g_gps_field[F_FIX],      value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "Ve tinh:",    2); field_init(&g_gps_field[F_SAT],      value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "HDOP:",       2); field_init(&g_gps_field[F_HDOP],     value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "Vi do:",      2); field_init(&g_gps_field[F_LAT],      value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "Kinh do:",    2); field_init(&g_gps_field[F_LON],      value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "Cao do (m):", 2); field_init(&g_gps_field[F_ALT],      value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "Toc do km/h:",2); field_init(&g_gps_field[F_SPD_KMH],  value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "Toc do knot:",2); field_init(&g_gps_field[F_SPD_KNOT], value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "Huong (do):", 2); field_init(&g_gps_field[F_COURSE],   value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "UTC time:",   2); field_init(&g_gps_field[F_TIME_UTC], value_x, y, value_w, 2); y += row_h;
//     draw_label(label_x, y, "Ngay:",       2); field_init(&g_gps_field[F_DATE],     value_x, y, value_w, 2); y += row_h;

//     draw_menu_bar(PAGE_GPS);
// }

// // Gọi khi đổi trang: dựng lại khung tĩnh tương ứng + ép các field vẽ lại giá trị
// static void switch_to_page(app_page_t page) {
//     g_current_page = page;
//     if (page == PAGE_BMI160) {
//         build_static_bmi160_page();
//         for (int i = 0; i < F_BMI_COUNT; i++) field_force_redraw(&g_bmi_field[i]);
//     } else {
//         build_static_gps_page();
//         for (int i = 0; i < F_GPS_COUNT; i++) field_force_redraw(&g_gps_field[i]);
//     }
//     g_drawn_page = page;
// }

// // ============================== CẬP NHẬT NỘI DUNG TRANG BMI160 ==============================

// static void update_bmi160_page(bool sensor_ok, const BMI160_Data *d) {
//     char buf[24];

//     field_update(&g_bmi_field[F_STATUS_BMI], sensor_ok ? "OK" : "LOI I2C",
//                  sensor_ok ? COLOR_VALUE : COLOR_VALUE_BAD);

//     if (!sensor_ok) return; // không có dữ liệu mới hợp lệ -> giữ nguyên các giá trị cũ trên màn hình

//     snprintf(buf, sizeof(buf), "%d", d->acc_x); field_update(&g_bmi_field[F_ACC_X], buf, COLOR_VALUE);
//     snprintf(buf, sizeof(buf), "%d", d->acc_y); field_update(&g_bmi_field[F_ACC_Y], buf, COLOR_VALUE);
//     snprintf(buf, sizeof(buf), "%d", d->acc_z); field_update(&g_bmi_field[F_ACC_Z], buf, COLOR_VALUE);
//     snprintf(buf, sizeof(buf), "%d", d->gyr_x); field_update(&g_bmi_field[F_GYR_X], buf, COLOR_VALUE);
//     snprintf(buf, sizeof(buf), "%d", d->gyr_y); field_update(&g_bmi_field[F_GYR_Y], buf, COLOR_VALUE);
//     snprintf(buf, sizeof(buf), "%d", d->gyr_z); field_update(&g_bmi_field[F_GYR_Z], buf, COLOR_VALUE);

//     // Công thức quy đổi nhiệt độ thô BMI160 -> độ C (theo datasheet): T(C) = 23 + raw/512
//     float temp_c = 23.0f + (float)d->temp / 512.0f;
//     snprintf(buf, sizeof(buf), "%.1f C", temp_c);
//     field_update(&g_bmi_field[F_TEMP], buf, COLOR_VALUE);
// }

// // ============================== CẬP NHẬT NỘI DUNG TRANG GPS ==============================

// static void update_gps_page(const neo6m_data_t *g) {
//     char buf[24];

//     const char *fix_txt =
//         (g->fix_quality == NEO6M_FIX_DGPS) ? "DGPS" :
//         (g->fix_quality == NEO6M_FIX_GPS)  ? "GPS"  : "NO FIX";
//     field_update(&g_gps_field[F_FIX], fix_txt,
//                  (g->fix_quality == NEO6M_FIX_INVALID) ? COLOR_VALUE_BAD : COLOR_VALUE);

//     snprintf(buf, sizeof(buf), "%u", g->satellites);
//     field_update(&g_gps_field[F_SAT], buf, COLOR_VALUE);

//     snprintf(buf, sizeof(buf), "%.2f", g->hdop);
//     field_update(&g_gps_field[F_HDOP], buf, COLOR_VALUE);

//     // Toạ độ và tốc độ chỉ thực sự có nghĩa khi đã fix vệ tinh, nhưng vẫn hiển thị
//     // giá trị thô gần nhất nếu có, để người dùng biết dữ liệu cuối cùng nhận được.
//     snprintf(buf, sizeof(buf), "%.6f", g->latitude);
//     field_update(&g_gps_field[F_LAT], buf, g->is_valid ? COLOR_VALUE : COLOR_VALUE_BAD);

//     snprintf(buf, sizeof(buf), "%.6f", g->longitude);
//     field_update(&g_gps_field[F_LON], buf, g->is_valid ? COLOR_VALUE : COLOR_VALUE_BAD);

//     snprintf(buf, sizeof(buf), "%.1f", g->altitude_m);
//     field_update(&g_gps_field[F_ALT], buf, COLOR_VALUE);

//     snprintf(buf, sizeof(buf), "%.1f", g->speed_kmh);
//     field_update(&g_gps_field[F_SPD_KMH], buf, COLOR_VALUE);

//     snprintf(buf, sizeof(buf), "%.1f", g->speed_knots);
//     field_update(&g_gps_field[F_SPD_KNOT], buf, COLOR_VALUE);

//     snprintf(buf, sizeof(buf), "%.1f", g->course_deg);
//     field_update(&g_gps_field[F_COURSE], buf, COLOR_VALUE);

//     snprintf(buf, sizeof(buf), "%02u:%02u:%02u", g->hour, g->minute, g->second);
//     field_update(&g_gps_field[F_TIME_UTC], buf, COLOR_VALUE);

//     snprintf(buf, sizeof(buf), "%02u/%02u/%04u", g->day, g->month, g->year);
//     field_update(&g_gps_field[F_DATE], buf, COLOR_VALUE);
// }

// // ============================== MAIN ==============================

// int main() {
//     stdio_init_all();
//     sleep_ms(500); // không bắt buộc chờ USB, để thiết bị tự chạy cả khi không cắm máy tính

//     printf("\n========== FLIGHT BLACK BOX - 2 PAGES UI ==========\n\n");

//     // ---------------- Khởi tạo màn hình ----------------
//     if (TFT_Init(&g_tft, TFT_SPI_PORT) != TFT_OK) {
//         printf(">>> LOI: Khoi tao TFT that bai!\n");
//         while (1) sleep_ms(1000);
//     }
//     TFT_Backlight(&g_tft, true);

//     // ---------------- Khởi tạo cảm ứng ----------------
//     if (XPT2046_Init(&g_touch, XPT2046_SPI_PORT) != XPT_OK) {
//         printf(">>> LOI: Khoi tao XPT2046 that bai!\n");
//         while (1) sleep_ms(1000);
//     }

//     // ---------------- Khởi tạo cảm biến BMI160 ----------------
//     BMI160_Config_t bmi_config = {
//         .accel_range = BMI_ACC_RANGE_2G,
//         .gyro_range  = BMI_GYR_RANGE_2000DPS,
//         .accel_odr   = BMI_ACC_CONFIG_DEFAULT,
//         .gyro_odr    = BMI_GYR_CONFIG_DEFAULT
//     };
//     bool bmi_ok = (BMI160_Init(&g_bmi, BMI160_I2C_PORT, BMI160_I2C_ADDR, &bmi_config) == BMI_OK);
//     printf(bmi_ok ? ">>> BMI160 OK\n" : ">>> BMI160 LOI KHOI TAO\n");

//     // ---------------- Khởi tạo GPS NEO-6M ----------------
//     memset(&g_gps_data, 0, sizeof(g_gps_data));
//     bool gps_ok = (NEO6M_Init(&g_gps_dev, NEO6M_UART_PORT, NEO6M_PIN_TX, NEO6M_PIN_RX, NEO6M_BAUDRATE) == NEO6M_OK);
//     printf(gps_ok ? ">>> NEO-6M OK\n" : ">>> NEO-6M LOI KHOI TAO\n");

//     // ---------------- Dựng trang mặc định ----------------
//     switch_to_page(PAGE_BMI160);

//     // ---------------- Vòng lặp chính ----------------
//     uint32_t last_bmi_read_ms   = 0;
//     uint32_t last_touch_ms      = 0;
//     const uint32_t BMI_PERIOD_MS    = 100;  // tốc độ đọc IMU (~10Hz)
//     const uint32_t TOUCH_DEBOUNCE_MS = 250;

//     BMI160_Data bmi_data;

//     while (1) {
//         uint32_t now = to_ms_since_boot(get_absolute_time());

//         // ----------- 1) Xử lý cảm ứng: chuyển trang theo yêu cầu, không tự đổi -----------
//         XPT2046_Update(&g_touch);
//         if (XPT2046_IsTouched(&g_touch) && (now - last_touch_ms > TOUCH_DEBOUNCE_MS)) {
//             int target = hit_test_menu(g_touch.x, g_touch.y);
//             if (target >= 0 && (app_page_t)target != g_current_page) {
//                 switch_to_page((app_page_t)target);
//             }
//             last_touch_ms = now;
//         }

//         // ----------- 2) Đọc GPS: NEO6M_Update cần được gọi liên tục để xử lý từng ký tự UART -----------
//         if (gps_ok) {
//             if (NEO6M_Update(&g_gps_dev, &g_gps_data) == NEO6M_OK) {
//                 // Có 1 câu NMEA mới được xử lý xong -> nếu đang ở trang GPS thì cập nhật
//                 if (g_current_page == PAGE_GPS) {
//                     update_gps_page(&g_gps_data);
//                 }
//             }
//         }

//         // ----------- 3) Đọc BMI160 theo chu kỳ cố định -----------
//         if (bmi_ok && (now - last_bmi_read_ms >= BMI_PERIOD_MS)) {
//             last_bmi_read_ms = now;
//             bool read_ok = (BMI160_ReadData(&g_bmi, &bmi_data) == BMI_OK);
//             if (g_current_page == PAGE_BMI160) {
//                 update_bmi160_page(read_ok, &bmi_data);
//             }
//         }

//         sleep_us(200); // nhường CPU, không chiếm dụng toàn bộ vòng lặp
//     }
// }





















//---------------------------test module sd card---------------------------




// =====================================================================================
//  MAIN TEST: DUNG LUONG THE + FILE LOG GIA LAP (GHI MOI GIAY -> DOC LAI -> XOA SAU 1 PHUT)
//  Cach dung: tam thoi thay noi dung src/main.cpp bang file nay, build & nap,
//  mo Serial Monitor de xem ket qua. Test xong nho khoi phuc lai main.cpp UI goc.
// =====================================================================================

// #include <stdio.h>
// #include <string.h>
// #include "pico/stdlib.h"

// #include "components/xpt2046/xpt2046.h"   // bắt buộc init trước để cấu hình bus SPI0
// #include "components/sdcard/sdcard.h"

// static xpt2046_dev_t g_touch;
// static sd_dev_t      g_sd;

// // In dung lượng dạng dễ đọc (KB/MB/GB) thay vì số byte thô khó nhìn
// static void print_capacity_human(const char *label, uint64_t bytes) {
//     double mb = (double)bytes / (1024.0 * 1024.0);
//     if (mb >= 1024.0) {
//         printf("    %s: %.2f GB (%llu byte)\n", label, mb / 1024.0, (unsigned long long)bytes);
//     } else {
//         printf("    %s: %.2f MB (%llu byte)\n", label, mb, (unsigned long long)bytes);
//     }
// }

// int main() {
//     stdio_init_all();
//     sleep_ms(3000); // chờ máy tính kịp mở cổng Serial

//     printf("\n========================================\n");
//     printf("  TEST: DUNG LUONG THE + FILE LOG SD\n");
//     printf("========================================\n\n");

//     // ---------------- Khởi tạo bus SPI0 + thẻ SD ----------------
//     printf("[INIT] Khoi tao bus SPI0 (qua XPT2046) + the SD...\n");
//     if (XPT2046_Init(&g_touch, XPT2046_SPI_PORT) != XPT_OK) {
//         printf("  >>> LOI: Khong khoi tao duoc bus SPI0!\n");
//         while (1) sleep_ms(1000);
//     }
//     if (SDCARD_Init(&g_sd, SDCARD_SPI_PORT) != SD_OK) {
//         printf("  >>> LOI: Khong khoi tao duoc the SD!\n");
//         while (1) sleep_ms(1000);
//     }
//     printf("  OK. Loai the: %s\n\n", (g_sd.card_type == SD_TYPE_SDHC) ? "SDHC/SDXC" : "SDSC");

//     // ---------------- Bước 1: đọc dung lượng thẻ (CSD) ----------------
//     printf("[1] Doc dung luong the (thanh ghi CSD)...\n");
//     uint64_t total_bytes = 0;
//     if (SDCARD_GetCapacityBytes(&g_sd, &total_bytes) == SD_OK) {
//         print_capacity_human("Tong dung luong vat ly cua the", total_bytes);
//     } else {
//         printf("    >>> LOI: Khong doc duoc dung luong the (CMD9 that bai).\n");
//     }

//     // LƯU Ý QUAN TRỌNG: vì driver này thao tác BLOCK THÔ (chưa có FAT32/FatFs), nên không thể
//     // biết "đã dùng/còn trống" của TOÀN THẺ theo đúng nghĩa hệ điều hành (điều đó đòi hỏi đọc
//     // bảng FAT thật). Ở đây mình chỉ báo cáo mức SỬ DỤNG TRONG VÙNG FILE LOG do driver tự quản lý
//     // (xem bước 2) như một con số tham khảo, KHÔNG đại diện cho toàn bộ thẻ.
//     uint64_t sdlog_region_bytes = (uint64_t)SDLOG_DATA_BLOCK_COUNT * SDCARD_BLOCK_SIZE;
//     print_capacity_human("Vung file log rieng (driver tu quan ly)", sdlog_region_bytes);
//     printf("\n");

//     // ---------------- Bước 2: tạo (reset) file log ----------------
//     printf("[2] Tao file log (vung block rieng, ghi tuan tu)...\n");
//     if (SDLOG_Create(&g_sd) != SD_OK) {
//         printf("    >>> LOI: Khong tao duoc file log, dung chuong trinh.\n");
//         while (1) sleep_ms(1000);
//     }
//     printf("\n");

//     // ---------------- Bước 3: ghi 1 tín hiệu mỗi giây trong 60 giây ----------------
//     printf("[3] Ghi tin hieu don gian moi giay, lien tuc trong 60 giay...\n\n");
//     const int LOG_DURATION_SEC = 60;
//     for (int sec = 1; sec <= LOG_DURATION_SEC; sec++) {
//         sleep_ms(1000);

//         uint32_t t_ms = to_ms_since_boot(get_absolute_time());
//         char line[SDLOG_RECORD_SIZE];
//         // Tín hiệu test đơn giản: "SIG,<so_thu_tu>,<thoi_gian_ms>"
//         snprintf(line, sizeof(line), "SIG,%d,%lu", sec, (unsigned long)t_ms);

//         SD_Status st = SDLOG_Append(&g_sd, line);
//         printf("  [%2d/%d] Ghi: \"%s\" -> %s\n",
//                sec, LOG_DURATION_SEC, line, (st == SD_OK) ? "OK" : "LOI");
//     }
//     printf("\n");

//     // ---------------- Bước 4: đọc lại toàn bộ nội dung file log, in ra terminal ----------------
//     printf("[4] Doc lai toan bo noi dung file log...\n\n");
//     uint32_t record_count = 0;
//     SDLOG_GetRecordCount(&g_sd, &record_count);
//     printf("  Tong so ban ghi hien co: %u\n", (unsigned)record_count);
//     SDLOG_PrintAll(&g_sd);
//     printf("\n");

//     // ---------------- Bước 5: xoá dữ liệu file log ----------------
//     printf("[5] Xoa du lieu file log...\n");
//     SDLOG_Delete(&g_sd);

//     // Xác nhận lại: đọc số bản ghi sau khi xoá, in lại nội dung (phải rỗng)
//     SDLOG_GetRecordCount(&g_sd, &record_count);
//     printf("  Sau khi xoa, so ban ghi con lai: %u\n", (unsigned)record_count);
//     SDLOG_PrintAll(&g_sd);

//     printf("\n========================================\n");
//     printf("   HOAN TAT TOAN BO QUY TRINH TEST!\n");
//     printf("========================================\n\n");

//     while (1) {
//         sleep_ms(2000);
//         printf("[IDLE] Da hoan tat, dang cho... (cat nguon hoac nap lai firmware khac)\n");
//     }
// }





// ------------------------------------test file -----------------------------------
// // =====================================================================================
// //  TEST FATFS – GHI FILE .CSV THẬT TRÊN THE SD
// //  Luong:
// //   1. Mount the (hoac tu format neu chua co FAT)
// //   2. Hien thi dung luong tong/da dung/con trong (f_getfree)
// //   3. Tao file "LOG.CSV" – ghi 60 ban ghi (moi giay 1 ban ghi)
// //   4. Doc lai toan bo file – in ra terminal
// //   5. Xoa file
// //   6. Xac nhan file da mat (f_stat tra FR_NO_FILE)
// // =====================================================================================

// #include <stdio.h>
// #include <string.h>
// #include "pico/stdlib.h"
// #include "fatfs/ff.h"
// #include "fatfs/diskio.h"

// // ---- ham tien ich in dung luong dang KB/MB/GB ---
// static void print_size(const char *label, uint64_t bytes)
// {
//     if (bytes >= 1024ULL * 1024 * 1024)
//         printf("  %-22s %.2f GB\n", label, (double)bytes / (1024.0*1024*1024));
//     else if (bytes >= 1024ULL * 1024)
//         printf("  %-22s %.2f MB\n", label, (double)bytes / (1024.0*1024));
//     else
//         printf("  %-22s %.2f KB\n", label, (double)bytes / 1024.0);
// }

// // ---- chuyen ma loi FatFs sang chuoi de doc ---
// static const char *fr_str(FRESULT r)
// {
//     switch (r) {
//     case FR_OK:              return "FR_OK";
//     case FR_DISK_ERR:        return "FR_DISK_ERR";
//     case FR_INT_ERR:         return "FR_INT_ERR";
//     case FR_NOT_READY:       return "FR_NOT_READY";
//     case FR_NO_FILE:         return "FR_NO_FILE";
//     case FR_NO_PATH:         return "FR_NO_PATH";
//     case FR_NO_FILESYSTEM:   return "FR_NO_FILESYSTEM";
//     case FR_MKFS_ABORTED:    return "FR_MKFS_ABORTED";
//     case FR_INVALID_NAME:    return "FR_INVALID_NAME";
//     default:                 return "FR_???";
//     }
// }

// int main()
// {
//     stdio_init_all();

//     // Chờ cho đến khi cổng USB CDC được máy tính nhận và mở
//     // (thay vì sleep cố định có thể in trước khi Serial Monitor kịp kết nối)
//     while (!stdio_usb_connected()) sleep_ms(100);

//     printf("\n========================================\n");
//     printf("   TEST FATFS – FILE CSV TREN THE SD\n");
//     printf("========================================\n\n");

//     // ----------------------------------------------------------------
//     // BUOC 1: mount – neu chua co FAT thi tu format (FAT32)
//     // ----------------------------------------------------------------
//     printf("[1] Mount the SD...\n");
//     static FATFS fs;
//     FRESULT fr = f_mount(&fs, "", 1);   // "" = drive 0, 1 = mount ngay lap tuc

//     if (fr == FR_NO_FILESYSTEM) {
//         printf("  The chua co FAT – dang format FAT32 (co the mat vai giay)...\n");
//         static BYTE work[FF_MAX_SS];    // buffer lam viec cho f_mkfs (512 byte)
//         MKFS_PARM opt = { FM_FAT32, 0, 0, 0, 0 };
//         fr = f_mkfs("", &opt, work, sizeof(work));
//         if (fr != FR_OK) {
//             printf("  >>> LOI format: %s  Kiem tra lai the SD.\n", fr_str(fr));
//             while (1) sleep_ms(2000);
//         }
//         printf("  Format xong. Mount lai...\n");
//         fr = f_mount(&fs, "", 1);
//     }

//     if (fr != FR_OK) {
//         printf("  >>> LOI mount: %s\n", fr_str(fr));
//         while (1) sleep_ms(2000);
//     }
//     printf("  Mount OK.\n\n");

//     // ----------------------------------------------------------------
//     // BUOC 2: dung luong
//     // ----------------------------------------------------------------
//     printf("[2] Dung luong the:\n");
//     DWORD free_clusters;
//     FATFS *pfs = &fs;
//     fr = f_getfree("", &free_clusters, &pfs);
//     if (fr == FR_OK) {
//         uint64_t total_bytes = (uint64_t)(pfs->n_fatent - 2) * pfs->csize * FF_MAX_SS;
//         uint64_t free_bytes  = (uint64_t)free_clusters       * pfs->csize * FF_MAX_SS;
//         uint64_t used_bytes  = total_bytes - free_bytes;
//         print_size("Tong dung luong:", total_bytes);
//         print_size("Da su dung:",      used_bytes);
//         print_size("Con trong:",       free_bytes);
//     } else {
//         printf("  LOI f_getfree: %s\n", fr_str(fr));
//     }
//     printf("\n");

//     // ----------------------------------------------------------------
//     // BUOC 3: tao / ghi file LOG.CSV
//     // ----------------------------------------------------------------
//     printf("[3] Ghi file LOG.CSV (60 ban ghi, moi giay 1 ban ghi)...\n");
//     static FIL fil;
//     fr = f_open(&fil, "LOG.CSV", FA_CREATE_ALWAYS | FA_WRITE);
//     if (fr != FR_OK) {
//         printf("  >>> LOI mo file de ghi: %s\n", fr_str(fr));
//         while (1) sleep_ms(2000);
//     }

//     // Dong tieu de
//     f_printf(&fil, "STT,TIME_MS,SIGNAL,NOTE\r\n");

//     for (int i = 1; i <= 60; i++) {
//         sleep_ms(1000);
//         uint32_t t_ms = to_ms_since_boot(get_absolute_time());
//         // Format CSV: so thu tu, thoi gian ms, gia tri tin hieu test, ghi chu
//         UINT bw;
//         char line[80];
//         snprintf(line, sizeof(line), "%d,%lu,SIG_%d,test\r\n",
//                  i, (unsigned long)t_ms, i);
//         f_write(&fil, line, strlen(line), &bw);
//         printf("  [%2d/60] %s", i, line);
//     }

//     f_close(&fil);
//     printf("  Dong file OK.\n\n");

//     // ----------------------------------------------------------------
//     // BUOC 4: doc lai toan bo file
//     // ----------------------------------------------------------------
//     printf("[4] Doc lai LOG.CSV:\n");
//     printf("  %-4s %-12s %-10s %s\n", "STT", "TIME_MS", "SIGNAL", "NOTE");
//     printf("  %s\n", "------------------------------------");

//     fr = f_open(&fil, "LOG.CSV", FA_READ);
//     if (fr != FR_OK) {
//         printf("  >>> LOI mo file de doc: %s\n", fr_str(fr));
//     } else {
//         char buf[80];
//         int line_num = 0;
//         while (f_gets(buf, sizeof(buf), &fil)) {
//             // Bo dong tieu de
//             if (line_num++ == 0) continue;
//             // Xoa ky tu xuong dong cuoi de in gon
//             int len = strlen(buf);
//             while (len > 0 && (buf[len-1] == '\r' || buf[len-1] == '\n'))
//                 buf[--len] = '\0';
//             printf("  %s\n", buf);
//         }
//         f_close(&fil);
//     }
//     printf("\n");

//     // ----------------------------------------------------------------
//     // BUOC 5: xoa file
//     // ----------------------------------------------------------------
//     printf("[5] Xoa LOG.CSV...\n");
//     fr = f_unlink("LOG.CSV");
//     printf("  f_unlink: %s\n\n", fr_str(fr));

//     // ----------------------------------------------------------------
//     // BUOC 6: xac nhan da xoa (f_stat phai tra FR_NO_FILE)
//     // ----------------------------------------------------------------
//     printf("[6] Xac nhan file da bi xoa (f_stat):\n");
//     FILINFO fno;
//     fr = f_stat("LOG.CSV", &fno);
//     if (fr == FR_NO_FILE) {
//         printf("  OK – file khong con ton tai (FR_NO_FILE).\n\n");
//     } else {
//         printf("  CANH BAO – f_stat tra: %s (du kien FR_NO_FILE).\n\n", fr_str(fr));
//     }

//     // Dung luong sau khi xoa
//     printf("  Dung luong sau khi xoa:\n");
//     fr = f_getfree("", &free_clusters, &pfs);
//     if (fr == FR_OK) {
//         uint64_t total_bytes = (uint64_t)(pfs->n_fatent - 2) * pfs->csize * FF_MAX_SS;
//         uint64_t free_bytes  = (uint64_t)free_clusters       * pfs->csize * FF_MAX_SS;
//         print_size("  Con trong:", free_bytes);
//         print_size("  Da dung:", total_bytes - free_bytes);
//     }

//     f_mount(NULL, "", 0);   // unmount sach se

//     printf("\n========================================\n");
//     printf("  HOAN TAT! File .csv da duoc tao, doc\n");
//     printf("  va xoa thanh cong qua FatFs + SD SPI.\n");
//     printf("========================================\n\n");

//     while (1) sleep_ms(5000);
// }








//------------------------------------test module sim ---------------------------














// #include <stdio.h>
// #include "pico/stdlib.h"
// #include "components/sim7680/sim7680.h"

// int main(void)
// {
//     // In log ra USB/UART0 mặc định (stdio) để theo dõi qua Serial Monitor
//     stdio_init_all();
//     sleep_ms(3000); // chờ mở cổng serial để không mất log đầu

//     printf("=== SIM7680 driver test ===\n");

//     sim7680_init();

//     printf("Dang doi module SIM khoi dong...\n");
//     if (!sim7680_wait_ready(15000)) {
//         printf("[LOI] Khong nhan duoc phan hoi AT tu module SIM!\n");
//         printf("-> Kiem tra lai day noi: SIM_TX->GPIO5, SIM_RX->GPIO4, GND chung, nguon cap du dong.\n");
//     } else {
//         printf("[OK] Module SIM da san sang.\n");
//     }

//     while (true) {
//         printf("\n--- Kiem tra module ---\n");

//         if (sim7680_test()) {
//             printf("AT test        : OK\n");
//         } else {
//             printf("AT test        : FAIL (khong phan hoi)\n");
//         }

//         char model[200];
//         if (sim7680_get_model(model, sizeof(model))) {
//             printf("Model          : %s\n", model);
//         } else {
//             printf("Model          : khong doc duoc\n");
//         }

//         char imei[32];
//         if (sim7680_get_imei(imei, sizeof(imei))) {
//             printf("IMEI           : %s\n", imei);
//         } else {
//             printf("IMEI           : khong doc duoc\n");
//         }

//         int rssi, ber;
//         if (sim7680_get_signal_quality(&rssi, &ber)) {
//             printf("Tin hieu (CSQ) : rssi=%d ber=%d\n", rssi, ber);
//         } else {
//             printf("Tin hieu (CSQ) : khong doc duoc\n");
//         }

//         int net_status;
//         if (sim7680_get_network_status(&net_status)) {
//             printf("Trang thai mang: %d (0=chua dang ky,1=da dang ky home,5=roaming)\n", net_status);
//         } else {
//             printf("Trang thai mang: khong doc duoc\n");
//         }

//         bool sim_ready;
//         if (sim7680_check_sim(&sim_ready)) {
//             printf("SIM             : %s\n", sim_ready ? "READY" : "KHONG NHAN DIEN DUOC");
//         } else {
//             printf("SIM             : khong doc duoc (module co the chua san sang)\n");
//         }

//         int cfun;
//         if (sim7680_get_radio_function(&cfun)) {
//             printf("Radio (CFUN)    : %d (0=tat song,1=bat day du,4=che do bay)\n", cfun);
//         } else {
//             printf("Radio (CFUN)    : khong doc duoc\n");
//         }

//         if (rssi == 99) {
//             printf(">> Goi y: rssi=99 nghia la khong do duoc tin hieu. Kiem tra: anten da cam chua,\n");
//             printf(">>        cam dung cong (chan MAIN), anten khong bi hong day.\n");
//         }

//         sleep_ms(5000);
//     }

//     return 0;
// }





//--------------------------------------test mqtt connection ---------------------------------------








#include <stdio.h>
#include "pico/stdlib.h"
#include "components/sim7680/sim7680.h"
#include "components/my_mqtt/my_mqtt.h"

int main() {
    // Khởi tạo toàn bộ cấu trúc Standard I/O (UART/USB Serial) để debug
    stdio_init_all();
    
    // Chờ cho đến khi cổng USB Serial được mở trên máy tính (Terminal/Serial Monitor)
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    sleep_ms(1000);

    printf("\n=== FLIGHT BLACK BOX - MQTT TEST ===\n\n");

    // Khởi tạo phần cứng UART giao tiếp giữa RP2040 và SIM7680
    printf("Đang chờ module SIM khởi động...\n");
    sim7680_init();

    // Chờ module SIM phản hồi lệnh AT cơ bản
    if (!sim7680_wait_ready(15000)) {
        printf("[LOI] Module SIM không phản hồi lệnh AT!\n");
        while(1) sleep_ms(1000);
    }

    // Kiểm tra và chờ đăng ký mạng di động (Thành công khi net_status là 1 hoặc 5)
    printf("Đang chờ đăng ký mạng...\n");
    int net_status = 0;
    uint32_t timeout = 30000;
    absolute_time_t deadline = make_timeout_time_ms(timeout);

    while (absolute_time_diff_us(get_absolute_time(), deadline) > 0) {
        sim7680_get_network_status(&net_status);
        if (net_status == 1 || net_status == 5) {
            printf("[OK] Đã đăng ký mạng!\n");
            break;
        }
        sleep_ms(2000);
    }

    if (net_status != 1 && net_status != 5) {
        printf("[LOI] Không đăng ký được mạng!\n");
        while(1) sleep_ms(1000);
    }

    // ====================== CẤU HÌNH & KẾT NỐI MQTT ======================
    printf("[MQTT] Khởi tạo...\n");
    
    if (mqtt_init() && mqtt_connect()) {
        // GIẢI PHÁP CHÍ MẠNG: Nghỉ 2 giây để dòng lệnh đồng bộ và stack mạng của SIM ổn định kết nối
        printf("[MQTT] Chờ stack mạng ổn định trước khi Subscribe...\n");
        sleep_ms(2000);

        // Tiến hành đăng ký Topic nhận lệnh điều khiển từ Server
        mqtt_subscribe(MQTT_TOPIC_COMMAND);

        int counter = 0;
        while (true) {
            // Liên tục gọi hàm process để kiểm tra dữ liệu URC (tin nhắn đến từ broker)
            mqtt_process();

            // Đóng gói chuỗi JSON Telemetry của Hộp đen hành trình
            char payload[128];
            snprintf(payload, sizeof(payload),
                     "{\"device\":\"blackbox\",\"counter\":%d,\"status\":\"online\"}", 
                     counter++);

            // Tiến hành Publish dữ liệu hành trình
            if (mqtt_publish(MQTT_TOPIC_TELEMETRY, payload, false)) {
                printf("[OK] Published: %s\n", payload);
            } else {
                printf("[FAIL] Publish failed\n");
            }

            // Giải pháp tránh block: Thay vì dùng sleep_ms(5000) gây chết luồng đọc UART,
            // ta chia nhỏ thời gian delay ra làm nhiều chu kỳ ngắn và chèn mqtt_process() vào giữa
            for (int i = 0; i < 50; i++) {
                mqtt_process(); // Quét cổng UART liên tục để tóm bản tin từ Server gửi xuống
                sleep_ms(100);  // 50 lần * 100ms = 5 giây giãn cách giữa các lần Publish
            }
        }
    } else {
        printf("[LOI] Không kết nối được MQTT Broker!\n");
        while(1) sleep_ms(1000);
    }

    return 0;
}
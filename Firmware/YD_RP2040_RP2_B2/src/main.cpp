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














// =====================================================================================
//  FLIGHT BLACK BOX - MAIN
//  Hệ thống đọc cảm biến BMI160 (IMU) + NEO-6M (GPS), hiển thị trên TFT 240x320 (ILI9341)
//  với 2 trang dữ liệu, chuyển trang bằng menu cảm ứng (XPT2046) ở đáy màn hình.
//
//  Nguyên tắc vẽ màn hình:
//   - Khi vào 1 trang: vẽ NHÃN (label) + khung tĩnh đúng 1 lần duy nhất.
//   - Mỗi vòng lặp: chỉ tính giá trị mới -> so sánh với giá trị cũ đã hiển thị
//     -> NẾU khác mới xoá vùng nhỏ (FillRect) và vẽ lại (DrawString) giá trị đó.
//     -> NẾU giống thì giữ nguyên, không vẽ lại (tránh nháy màn hình / tốn thời gian SPI).
// =====================================================================================

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
// },





















//---------------------------test module sd card---------------------------




// =====================================================================================
//  MAIN KIỂM TRA RIÊNG MODULE SD CARD (sdcard.h/.cpp)
//  Mục đích: xác minh phần cứng + driver hoạt động đúng TRƯỚC khi tích hợp vào main chính.
//  Cách dùng: tạm thời thay nội dung file src/main.cpp bằng file này, build & nạp,
//  mở Serial Monitor (USB CDC) để xem kết quả. Sau khi test xong, khôi phục lại main.cpp gốc.
// =====================================================================================

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "components/xpt2046/xpt2046.h"   // bắt buộc init trước để cấu hình bus SPI0
#include "components/sdcard/sdcard.h"

// Dùng block số 10000 để test (đủ xa block 0/MBR, an toàn không phá dữ liệu sẵn có trên thẻ)
#define TEST_BLOCK_ADDR   10000

static xpt2046_dev_t g_touch;
static sd_dev_t      g_sd;

// In ra 16 byte đầu của buffer dạng HEX để dễ quan sát khi debug
static void print_hex_preview(const uint8_t *buf, const char *label) {
    printf("    %s (16 byte dau): ", label);
    for (int i = 0; i < 16; i++) printf("%02X ", buf[i]);
    printf("\n");
}

int main() {
    stdio_init_all();
    sleep_ms(3000); // chờ máy tính kịp mở cổng Serial trước khi in log (không bắt buộc, có thể bỏ)

    printf("\n========================================\n");
    printf("   KIEM TRA MODULE THE NHO SD (SPI0)\n");
    printf("========================================\n\n");

    // ---------------- Bước 1: khởi tạo bus SPI0 thông qua XPT2046 ----------------
    // (Vì SD card dùng chung bus SPI0 với cảm ứng, driver SD không tự cấu hình
    //  GPIO_FUNC_SPI cho MISO/MOSI/SCK, nên bắt buộc phải gọi XPT2046_Init() trước)
    printf("[1] Khoi tao bus SPI0 (qua XPT2046_Init)...\n");
    if (XPT2046_Init(&g_touch, XPT2046_SPI_PORT) != XPT_OK) {
        printf("    >>> LOI: Khong the khoi tao bus SPI0!\n");
        while (1) sleep_ms(1000);
    }
    printf("    OK.\n\n");

    // ---------------- Bước 2: khởi tạo thẻ SD ----------------
    printf("[2] Khoi tao the SD (CMD0 -> CMD8 -> ACMD41 -> CMD58)...\n");
    SD_Status init_status = SDCARD_Init(&g_sd, SDCARD_SPI_PORT);
    if (init_status != SD_OK) {
        printf("    >>> THAT BAI! Ma loi = %d\n", init_status);
        printf("    Kiem tra lai: day noi MISO/MOSI/SCK/CS, the co duoc cam chac khong,\n");
        printf("    nguon 3.3V cho module SD co du dong khong.\n");
        while (1) {
            sleep_ms(2000);
            printf("    [Dang doi...] Rut/cam lai the de thu lai (chua ho tro hot-plug tu dong).\n");
        }
    }
    printf("    OK. Loai the: %s\n\n",
           (g_sd.card_type == SD_TYPE_SDHC) ? "SDHC/SDXC" : "SDSC");

    // ---------------- Bước 3: kiểm tra phản hồi (CMD58 - đọc OCR) ----------------
    printf("[3] Kiem tra ket noi (CMD58 - doc OCR)...\n");
    if (SDCARD_CheckConnection(&g_sd) == SD_OK) {
        printf("    OK. The van dang phan hoi binh thuong.\n\n");
    } else {
        printf("    >>> CANH BAO: The khong phan hoi CMD58!\n\n");
    }

    // ---------------- Bước 4: test ghi + đọc lại 1 block, so sánh dữ liệu ----------------
    printf("[4] Test ghi/doc block #%d (512 byte)...\n", TEST_BLOCK_ADDR);

    uint8_t write_buf[SDCARD_BLOCK_SIZE];
    uint8_t read_buf[SDCARD_BLOCK_SIZE];

    // Tạo mẫu dữ liệu test dễ nhận biết: byte thứ i = (i % 256), khác hẳn dữ liệu rác mặc định
    for (int i = 0; i < SDCARD_BLOCK_SIZE; i++) write_buf[i] = (uint8_t)(i & 0xFF);

    printf("    -> Dang ghi...\n");
    SD_Status write_status = SDCARD_WriteBlock(&g_sd, TEST_BLOCK_ADDR, write_buf);
    if (write_status != SD_OK) {
        printf("    >>> LOI GHI! Ma loi = %d\n", write_status);
    } else {
        printf("    Ghi thanh cong.\n");
    }

    memset(read_buf, 0, SDCARD_BLOCK_SIZE); // xoá sạch buffer đọc để chắc chắn không "ăn gian" kết quả
    printf("    -> Dang doc lai...\n");
    SD_Status read_status = SDCARD_ReadBlock(&g_sd, TEST_BLOCK_ADDR, read_buf);
    if (read_status != SD_OK) {
        printf("    >>> LOI DOC! Ma loi = %d\n", read_status);
    } else {
        printf("    Doc thanh cong.\n");
    }

    print_hex_preview(write_buf, "Da ghi  ");
    print_hex_preview(read_buf,  "Da doc  ");

    bool data_match = (write_status == SD_OK && read_status == SD_OK &&
                        memcmp(write_buf, read_buf, SDCARD_BLOCK_SIZE) == 0);

    printf("\n========================================\n");
    if (data_match) {
        printf("   KET QUA: THE SD HOAT DONG TOT!\n");
        printf("   (Du lieu ghi va doc lai khop nhau 100%%)\n");
    } else {
        printf("   KET QUA: THE SD CO VAN DE!\n");
        printf("   (Du lieu khong khop hoac thao tac that bai)\n");
    }
    printf("========================================\n\n");

    // ---------------- Bước 5: lặp lại kiểm tra định kỳ để theo dõi độ ổn định ----------------
    printf("Bat dau vong lap kiem tra dinh ky moi 5 giay (CMD58)...\n");
    printf("Quan sat: neu the bi long/mat ket noi giua chung, dong nay se bao loi ngay.\n\n");

    uint32_t round = 0;
    while (1) {
        sleep_ms(5000);
        round++;
        SD_Status check = SDCARD_CheckConnection(&g_sd);
        printf("[Vong %lu] CheckConnection: %s\n",
               round, (check == SD_OK) ? "OK" : "MAT KET NOI / LOI");
    }
}
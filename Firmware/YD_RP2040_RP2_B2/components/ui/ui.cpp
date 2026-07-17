#include "ui.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#include "tft.h"
#include "xpt2046.h"
#include "neo6m.h"
#include "algorithm_bmi160.h"

// ================      Trạng thái nội bộ của UI         ==============
static tft_dev_t    s_tft;        // device handle màn hình ILI9341 (SPI1)
static xpt2046_dev_t s_touch;     // device handle cảm ứng XPT2046 (SPI0)
static int           s_cur_page = 1; // trang đang hiển thị (1=GPS, 2=IMU, 3=SD)
static SemaphoreHandle_t s_mutex_spi0 = NULL; // sao chép từ TaskParams, dùng nội bộ

// ================      TOP BAR (thanh trạng thái trên)  ==============
// Static: Nền, nhãn cố định (SD, SIM, PIN) - chỉ vẽ 1 lần khi boot hoặc đổi trang
static void UI_DrawTopBar_Static(void) {
    TFT_FillRect(&s_tft, 0, 0, TFT_WIDTH, 30, TFT_COLOR_BLUE);
    TFT_DrawString(&s_tft, 5,  10, "SD",     TFT_COLOR_WHITE, TFT_COLOR_BLUE, 1);
    TFT_DrawString(&s_tft, 35, 10, "SIM:OK", TFT_COLOR_WHITE, TFT_COLOR_BLUE, 1);
    TFT_DrawString(&s_tft, 190,10, "[85%]",  TFT_COLOR_WHITE, TFT_COLOR_BLUE, 1);
}

// Dynamic: Chỉ cập nhật đồng hồ UTC từ GPS mỗi giây - không xóa toàn bộ bar
static void UI_DrawTopBar_Dynamic(void) {
    neo6m_data_t gps;
    if (GPS_GetLastData(&gps) && gps.is_valid) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%02u:%02u ", (unsigned)gps.hour, (unsigned)gps.minute);
        TFT_DrawString(&s_tft, 100, 10, buf, TFT_COLOR_WHITE, TFT_COLOR_BLUE, 1);
    } else {
        TFT_DrawString(&s_tft, 100, 10, "--:-- ", TFT_COLOR_WHITE, TFT_COLOR_BLUE, 1);
    }
}

// ================      BOTTOM TABS (nút bấm 3 trang)    ==============
// Vẽ lại toàn bộ tab bar khi đổi trang (highlight tab đang chọn bằng màu đỏ)
static void UI_DrawBottomTabs(void) {
    TFT_FillRect(&s_tft, 0, TFT_HEIGHT - 50, TFT_WIDTH, 50, TFT_COLOR_BLACK);

    // Đường kẻ phân cách: 1 ngang + 2 dọc
    TFT_FillRect(&s_tft, 0,               TFT_HEIGHT - 50, TFT_WIDTH, 2,  TFT_COLOR_WHITE);
    TFT_FillRect(&s_tft, TFT_WIDTH / 3,   TFT_HEIGHT - 50, 2,         50, TFT_COLOR_WHITE);
    TFT_FillRect(&s_tft, 2 * TFT_WIDTH / 3, TFT_HEIGHT - 50, 2,       50, TFT_COLOR_WHITE);

    uint16_t c1 = (s_cur_page == 1) ? TFT_COLOR_RED : TFT_COLOR_WHITE;
    uint16_t c2 = (s_cur_page == 2) ? TFT_COLOR_RED : TFT_COLOR_WHITE;
    uint16_t c3 = (s_cur_page == 3) ? TFT_COLOR_RED : TFT_COLOR_WHITE;

    TFT_DrawString(&s_tft, 20,  TFT_HEIGHT - 35, "GPS", c1, TFT_COLOR_BLACK, 2);
    TFT_DrawString(&s_tft, 100, TFT_HEIGHT - 35, "IMU", c2, TFT_COLOR_BLACK, 2);
    TFT_DrawString(&s_tft, 185, TFT_HEIGHT - 35, "SD",  c3, TFT_COLOR_BLACK, 2);
}

// ================      PAGE 1: GPS (NEO-6M)              ==============
// Static: Nhãn cột (LAT, LON, ALT ...) - chỉ vẽ khi mới vào trang
static void UI_DrawPage1_Static(void) {
    TFT_FillRect(&s_tft, 0, 30, TFT_WIDTH, TFT_HEIGHT - 80, TFT_COLOR_BLACK);
    TFT_DrawString(&s_tft, 20, 40,  "GPS (NEO-6M) INFO", TFT_COLOR_YELLOW, TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 80,  "* LAT :",            TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 100, "* LON :",            TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 120, "* ALT :",            TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 150, "* SPD :",            TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 120,150, "* HDG :",            TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 180, "* TIME:",            TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 200, "* SATS:",            TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
}

// Dynamic: Chỉ cập nhật vùng số liệu thay đổi - không xóa nhãn cố định
static void UI_DrawPage1_Dynamic(void) {
    neo6m_data_t gps;
    if (GPS_GetLastData(&gps)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "  %.6f N       ", gps.latitude);
        TFT_DrawString(&s_tft, 60, 80, buf, TFT_COLOR_CYAN, TFT_COLOR_BLACK, 1);

        snprintf(buf, sizeof(buf), "  %.6f E       ", gps.longitude);
        TFT_DrawString(&s_tft, 60, 100, buf, TFT_COLOR_CYAN, TFT_COLOR_BLACK, 1);

        snprintf(buf, sizeof(buf), "  %.1f m         ", gps.altitude_m);
        TFT_DrawString(&s_tft, 60, 120, buf, TFT_COLOR_CYAN, TFT_COLOR_BLACK, 1);

        snprintf(buf, sizeof(buf), "%.1f km/h ", gps.speed_kmh);
        TFT_DrawString(&s_tft, 60, 150, buf, TFT_COLOR_CYAN, TFT_COLOR_BLACK, 1);

        snprintf(buf, sizeof(buf), "%.0f deg  ", gps.course_deg);
        TFT_DrawString(&s_tft, 170, 150, buf, TFT_COLOR_CYAN, TFT_COLOR_BLACK, 1);

        snprintf(buf, sizeof(buf), "  %02u:%02u:%02u UTC   ",
                 (unsigned)gps.hour, (unsigned)gps.minute, (unsigned)gps.second);
        TFT_DrawString(&s_tft, 60, 180, buf, TFT_COLOR_CYAN, TFT_COLOR_BLACK, 1);

        snprintf(buf, sizeof(buf), "  %02u        ", (unsigned)gps.satellites);
        TFT_DrawString(&s_tft, 60, 200, buf, TFT_COLOR_CYAN, TFT_COLOR_BLACK, 1);
    }
}

// ================      PAGE 2: IMU (BMI160)              ==============
// Static: Nhãn cố định
static void UI_DrawPage2_Static(void) {
    TFT_FillRect(&s_tft, 0, 30, TFT_WIDTH, TFT_HEIGHT - 80, TFT_COLOR_BLACK);
    TFT_DrawString(&s_tft, 20, 40,  "BMI160 IMU INFO",  TFT_COLOR_YELLOW, TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 80,  "* PITCH :",        TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 100, "* ROLL  :",        TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 120, "* G-MAG :",        TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 160, "ACC (g) :",        TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 180, "GYR(dps):",        TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
}

// Dynamic: Chỉ cập nhật số liệu cảm biến thay đổi liên tục
static void UI_DrawPage2_Dynamic(void) {
    BMI160_Physical_t bmi;
    if (BMI160_GetLastData(&bmi)) {
        char buf[64];
        snprintf(buf, sizeof(buf), " %+6.1f deg    ", bmi.acc_y_g * 90.0f);
        TFT_DrawString(&s_tft, 80, 80, buf, TFT_COLOR_CYAN, TFT_COLOR_BLACK, 1);

        snprintf(buf, sizeof(buf), " %+6.1f deg    ", bmi.acc_x_g * 90.0f);
        TFT_DrawString(&s_tft, 80, 100, buf, TFT_COLOR_CYAN, TFT_COLOR_BLACK, 1);

        snprintf(buf, sizeof(buf), " %5.2f G      ", bmi.acc_magnitude_g);
        TFT_DrawString(&s_tft, 80, 120, buf, TFT_COLOR_CYAN, TFT_COLOR_BLACK, 1);

        snprintf(buf, sizeof(buf), "X:%+.1f Y:%+.1f Z:%+.1f  ",
                 bmi.acc_x_g, bmi.acc_y_g, bmi.acc_z_g);
        TFT_DrawString(&s_tft, 80, 160, buf, TFT_COLOR_GREEN, TFT_COLOR_BLACK, 1);

        snprintf(buf, sizeof(buf), "X:%+.0f Y:%+.0f Z:%+.0f  ",
                 bmi.gyr_x_dps, bmi.gyr_y_dps, bmi.gyr_z_dps);
        TFT_DrawString(&s_tft, 80, 180, buf, TFT_COLOR_GREEN, TFT_COLOR_BLACK, 1);
    }
}

// ================      PAGE 3: SD CARD                   ==============
// Static: Thông tin file system (cố định, không cập nhật theo thời gian)
static void UI_DrawPage3_Static(void) {
    TFT_FillRect(&s_tft, 0, 30, TFT_WIDTH, TFT_HEIGHT - 80, TFT_COLOR_BLACK);
    TFT_DrawString(&s_tft, 40, 40,  "STORAGE STATUS",               TFT_COLOR_YELLOW, TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 70,  "* SD CARD: DETECTED",          TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 90,  "* FILE SYSTEM: FAT32",         TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 130, "* CAPACITY:",                  TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 150, "  [||||||||||||||||||.......]", TFT_COLOR_CYAN,   TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 170, "  Used: 4.2 GB / Free: 11.8 GB", TFT_COLOR_WHITE, TFT_COLOR_BLACK, 1);
    TFT_DrawString(&s_tft, 10, 210, "* STATUS:",                    TFT_COLOR_WHITE,  TFT_COLOR_BLACK, 1);
}

// Dynamic: Chỉ nhấp nháy dòng trạng thái "LOGGING..." mỗi giây
static void UI_DrawPage3_Dynamic(void) {
    static bool blink = false;
    blink = !blink;
    uint16_t color = blink ? TFT_COLOR_RED : TFT_COLOR_WHITE;
    TFT_DrawString(&s_tft, 10, 230, "  >> LOGGING...       ", color, TFT_COLOR_BLACK, 1);
}

// ================      TASK ENTRY POINT                  ==============
void TaskUI(void *pvParameters) {
    UI_TaskParams_t *params = (UI_TaskParams_t *)pvParameters;
    s_mutex_spi0 = params ? params->mutex_spi0 : NULL;

    // B1: Khởi tạo phần cứng
    if (TFT_Init(&s_tft, TFT_SPI_PORT) != TFT_OK) {
        printf("[TaskUI] TFT_Init that bai!\n");
    }

    if (s_mutex_spi0) xSemaphoreTake(s_mutex_spi0, portMAX_DELAY);
    if (XPT2046_Init(&s_touch, XPT2046_SPI_PORT) != XPT_OK) {
        printf("[TaskUI] XPT2046_Init that bai!\n");
    }
    if (s_mutex_spi0) xSemaphoreGive(s_mutex_spi0);

    // B2: Vẽ màn hình khởi động (Page 1 mặc định)
    TFT_FillScreen(&s_tft, TFT_COLOR_BLACK);
    UI_DrawTopBar_Static();
    UI_DrawTopBar_Dynamic();
    UI_DrawPage1_Static();
    UI_DrawPage1_Dynamic();
    UI_DrawBottomTabs();

    int last_page = s_cur_page;
    TickType_t lastWake = xTaskGetTickCount();
    uint32_t redraw_counter = 0;

    // B3: Vòng lặp chính - 100ms/chu kỳ, cập nhật số liệu mỗi 1 giây (10 chu kỳ)
    for (;;) {
        // Đọc cảm ứng (lấy Mutex trước khi dùng SPI0)
        bool touched = false;
        if (s_mutex_spi0 && xSemaphoreTake(s_mutex_spi0, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (XPT2046_Update(&s_touch) == XPT_OK) {
                touched = s_touch.is_touched;
            }
            xSemaphoreGive(s_mutex_spi0);
        }

        // Xử lý sự kiện bấm (vùng 50px cuối màn hình = 3 nút tab)
        if (touched && s_touch.y > TFT_HEIGHT - 50) {
            if      (s_touch.x < TFT_WIDTH / 3)       s_cur_page = 1;
            else if (s_touch.x < 2 * TFT_WIDTH / 3)   s_cur_page = 2;
            else                                        s_cur_page = 3;
        }

        // Chuyển trang: vẽ lại toàn bộ phần Static + Dynamic của trang mới
        if (s_cur_page != last_page) {
            if      (s_cur_page == 1) { UI_DrawPage1_Static(); UI_DrawPage1_Dynamic(); }
            else if (s_cur_page == 2) { UI_DrawPage2_Static(); UI_DrawPage2_Dynamic(); }
            else                      { UI_DrawPage3_Static(); UI_DrawPage3_Dynamic(); }
            UI_DrawBottomTabs();
            last_page = s_cur_page;
        } else {
            // Cùng trang: cứ 10 chu kỳ (1 giây) mới cập nhật số liệu động
            redraw_counter++;
            if (redraw_counter >= 10) {
                redraw_counter = 0;
                UI_DrawTopBar_Dynamic();
                if      (s_cur_page == 1) UI_DrawPage1_Dynamic();
                else if (s_cur_page == 2) UI_DrawPage2_Dynamic();
                else                      UI_DrawPage3_Dynamic();
            }
        }

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(100));
    }
}

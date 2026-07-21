#include "task_sdcard.h"
#include "task_bmi160.h"
#include "task_gps.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

static FATFS g_fatfs;
static FIL g_sdlog_file;
static bool g_sdlog_file_open = false;
static char g_sdlog_current_name[SDLOG_FILENAME_MAXLEN] = {0};
static uint32_t g_sdlog_record_count = 0;

static void SDLog_BuildFileName(char *out, size_t out_len, const neo6m_data_t *gps,
                                uint32_t minutes) {
    if (out == NULL || out_len == 0) {
        return;
    }

    if (gps && gps->is_valid) {
        snprintf(out, out_len, "%02u%02u%02u.CSV",
                 (unsigned)gps->day,
                 (unsigned)gps->hour,
                 (unsigned)gps->minute);
    } else {
        snprintf(out, out_len, "B%06u.CSV", (unsigned)minutes);
    }
    out[out_len - 1] = '\0';
}

static SD_Status SDLog_OpenNewFile(const char *filename) {
    if (g_mutex_spi0)
        xSemaphoreTake(g_mutex_spi0, portMAX_DELAY);

    if (g_sdlog_file_open) {
        FSIZE_t size_before_close = f_size(&g_sdlog_file);
        f_close(&g_sdlog_file);
        g_sdlog_file_open = false;
        printf("[TaskSDCard][DEBUG] Da dong file '%s': %lu dong du lieu, %lu byte.\n",
               g_sdlog_current_name, (unsigned long)g_sdlog_record_count,
               (unsigned long)size_before_close);
    }

    FRESULT fr = f_open(&g_sdlog_file, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        if (g_mutex_spi0)
            xSemaphoreGive(g_mutex_spi0);
        printf("[TaskSDCard][DEBUG] LOI f_open('%s') fr=%d -> KHONG mo duoc file moi!\n",
               filename, fr);
        return SD_ERROR;
    }

    g_sdlog_file_open = true;
    g_sdlog_record_count = 0;
    strncpy(g_sdlog_current_name, filename, sizeof(g_sdlog_current_name) - 1);
    g_sdlog_current_name[sizeof(g_sdlog_current_name) - 1] = '\0';

    static const char header[] =
        "time_s,acc_x_g,acc_y_g,acc_z_g,acc_mag_g,gyr_x_dps,gyr_y_dps,gyr_z_dps,"
        "lat,lon,alt_m,speed_kmh,sat,fix\r\n";
    UINT bw = 0;
    FRESULT fw = f_write(&g_sdlog_file, header, sizeof(header) - 1, &bw);
    f_sync(&g_sdlog_file);
    if (g_mutex_spi0)
        xSemaphoreGive(g_mutex_spi0);

    printf("[TaskSDCard][DEBUG] >>> Da MO file log MOI: '%s' (header %u/%u byte, fw=%d)\n",
           filename, (unsigned)bw, (unsigned)(sizeof(header) - 1), fw);
    return SD_OK;
}

void TaskSDCard(void *pvParameters) {
    (void)pvParameters;
    printf("[TaskSDCard] Khoi tao FatFs...\n");

    FRESULT fr;
    if (g_mutex_spi0)
        xSemaphoreTake(g_mutex_spi0, portMAX_DELAY);
    while ((fr = f_mount(&g_fatfs, "", 1)) != FR_OK) {
        if (g_mutex_spi0)
            xSemaphoreGive(g_mutex_spi0);
        printf("[TaskSDCard] Mount FAT that bai (fr=%d), thu lai...\n", fr);
        vTaskDelay(pdMS_TO_TICKS(SDCARD_MOUNT_RETRY_MS));
        if (g_mutex_spi0)
            xSemaphoreTake(g_mutex_spi0, portMAX_DELAY);
    }
    if (g_mutex_spi0)
        xSemaphoreGive(g_mutex_spi0);
    printf("[TaskSDCard] Mount FAT thanh cong.\n");

    uint32_t cycle_count = 0;
    uint32_t elapsed_s = 0;
    char filename[SDLOG_FILENAME_MAXLEN];
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        BMI160_Physical_t bmi = {0};
        neo6m_data_t gps = {0};
        bool has_bmi = BMI160_GetLastData(&bmi);
        bool has_gps = GPS_GetLastData(&gps);

        if (cycle_count % SDCARD_ROTATE_EVERY_N_CYCLES == 0) {
            printf("[TaskSDCard][DEBUG] Toi han xoay file (elapsed=%lus, phut thu %lu) -> tao file moi...\n",
                   (unsigned long)elapsed_s, (unsigned long)(elapsed_s / 60));
            SDLog_BuildFileName(filename, sizeof(filename), &gps, elapsed_s / 60);
            SDLog_OpenNewFile(filename);
        }

        if (g_sdlog_file_open) {
            char line[160];
            int len = snprintf(
                line, sizeof(line),
                "%lu,%.3f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.6f,%.6f,%.1f,%.1f,%u,%d\r\n",
                (unsigned long)elapsed_s, has_bmi ? bmi.acc_x_g : 0.0f,
                has_bmi ? bmi.acc_y_g : 0.0f, has_bmi ? bmi.acc_z_g : 0.0f,
                has_bmi ? bmi.acc_magnitude_g : 0.0f, has_bmi ? bmi.gyr_x_dps : 0.0f,
                has_bmi ? bmi.gyr_y_dps : 0.0f, has_bmi ? bmi.gyr_z_dps : 0.0f,
                has_gps ? gps.latitude : 0.0f, has_gps ? gps.longitude : 0.0f,
                has_gps ? gps.altitude_m : 0.0f, has_gps ? gps.speed_kmh : 0.0f,
                has_gps ? (unsigned)gps.satellites : 0u,
                has_gps ? (int)gps.fix_quality : -1);

            UINT bw = 0;
            if (g_mutex_spi0)
                xSemaphoreTake(g_mutex_spi0, portMAX_DELAY);
            FRESULT wr = f_write(&g_sdlog_file, line, (UINT)len, &bw);
            if (wr != FR_OK || bw != (UINT)len) {
                if (g_mutex_spi0)
                    xSemaphoreGive(g_mutex_spi0);
                printf("[TaskSDCard][DEBUG] LOI ghi file '%s'! fr=%d (bw=%u/%d)\n",
                       g_sdlog_current_name, wr, (unsigned)bw, len);
            } else {
                f_sync(&g_sdlog_file);
                g_sdlog_record_count++;
                if (g_mutex_spi0)
                    xSemaphoreGive(g_mutex_spi0);
            }
        }

        cycle_count++;
        elapsed_s++;
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SDCARD_WRITE_PERIOD_MS));
    }
}

#include "task_sdcard.h"
#include "task_bmi160.h"
#include "task_gps.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

#include "shared_data.h"

static FATFS g_fatfs;
static FIL g_sdlog_file;
static bool g_sdlog_file_open = false;
static char g_sdlog_current_name[SDLOG_FILENAME_MAXLEN] = {0};
static uint32_t g_sdlog_record_count = 0;

// ========== Thresholds for Card Events ==========
#define CARD_NEAR_FULL_PCT  10   // Cảnh báo khi < 10% free
#define CARD_PERIODIC_CYCLES 60  // Publish card info mỗi 60 chu kỳ = 60s

static void publish_card_event(const char *event_type, const char *filename,
                               uint32_t records, uint32_t bytes,
                               uint32_t free_mb, uint8_t free_pct) {
    if (!g_mqtt_event_queue) return;

    mqtt_event_t evt = {0};
    evt.type = 1;  // card_event type
    
    if (strcmp(event_type, "start") == 0) {
        snprintf(evt.payload, sizeof(evt.payload),
                 "{\"ts\":%u,\"event\":\"start\",\"filename\":\"%s\"}",
                 xTaskGetTickCount() / 1000, filename);
    } else if (strcmp(event_type, "periodic") == 0) {
        snprintf(evt.payload, sizeof(evt.payload),
                 "{\"ts\":%u,\"event\":\"periodic\",\"filename\":\"%s\","
                 "\"records\":%lu,\"bytes\":%lu,\"free_mb\":%lu,\"free_pct\":%u}",
                 xTaskGetTickCount() / 1000, filename,
                 (unsigned long)records, (unsigned long)bytes,
                 (unsigned long)free_mb, (unsigned)free_pct);
    } else if (strcmp(event_type, "stop") == 0) {
        snprintf(evt.payload, sizeof(evt.payload),
                 "{\"ts\":%u,\"event\":\"stop\",\"filename\":\"%s\","
                 "\"records\":%lu,\"bytes\":%lu}",
                 xTaskGetTickCount() / 1000, filename,
                 (unsigned long)records, (unsigned long)bytes);
    } else if (strcmp(event_type, "near_full") == 0) {
        snprintf(evt.payload, sizeof(evt.payload),
                 "{\"ts\":%u,\"event\":\"near_full\",\"free_mb\":%lu,\"free_pct\":%u}",
                 xTaskGetTickCount() / 1000, (unsigned long)free_mb, (unsigned)free_pct);
    } else if (strcmp(event_type, "full") == 0) {
        snprintf(evt.payload, sizeof(evt.payload),
                 "{\"ts\":%u,\"event\":\"full\",\"free_mb\":0,\"free_pct\":0}",
                 xTaskGetTickCount() / 1000);
    }

    xQueueSend(g_mqtt_event_queue, &evt, 0);
    printf("[TaskSDCard] Card event published: %s\n", event_type);
}

static uint32_t get_card_free_space_pct(void) {
    // TODO: Implement FatFs f_getfree() to get actual free space
    // For now return dummy value
    return 85;
}

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
        
        // Publish "stop" event
        publish_card_event("stop", g_sdlog_current_name, g_sdlog_record_count, (uint32_t)size_before_close, 0, 0);
        
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

    // Publish "start" event
    publish_card_event("start", filename, 0, 0, 0, 0);

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
        record_error(ERROR_SDCARD_MOUNT);
        vTaskDelay(pdMS_TO_TICKS(SDCARD_MOUNT_RETRY_MS));
        if (g_mutex_spi0)
            xSemaphoreTake(g_mutex_spi0, portMAX_DELAY);
    }
    if (g_mutex_spi0)
        xSemaphoreGive(g_mutex_spi0);
    printf("[TaskSDCard] Mount FAT thanh cong.\n");

    uint32_t cycle_count = 0;
    uint32_t elapsed_s = 0;
    uint8_t last_free_pct = 100;
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
                record_error(ERROR_SDCARD_WRITE);
                // Publish "full" event on write failure
                publish_card_event("full", g_sdlog_current_name, g_sdlog_record_count, 0, 0, 0);
            } else {
                f_sync(&g_sdlog_file);
                g_sdlog_record_count++;
                if (g_mutex_spi0)
                    xSemaphoreGive(g_mutex_spi0);
            }
        }

        // Publish periodic card status mỗi CARD_PERIODIC_CYCLES
        if (g_sdlog_file_open && cycle_count % CARD_PERIODIC_CYCLES == 0) {
            uint32_t free_pct = get_card_free_space_pct();
            FSIZE_t file_size = f_size(&g_sdlog_file);
            publish_card_event("periodic", g_sdlog_current_name,
                             g_sdlog_record_count, (uint32_t)file_size,
                             0, (uint8_t)free_pct);  // TODO: Get actual free_mb
        }

        // Check for near_full or full conditions
        uint32_t current_free_pct = get_card_free_space_pct();
        if (current_free_pct < CARD_NEAR_FULL_PCT && last_free_pct >= CARD_NEAR_FULL_PCT) {
            publish_card_event("near_full", g_sdlog_current_name, g_sdlog_record_count, 0, 0, (uint8_t)current_free_pct);
        }
        last_free_pct = current_free_pct;

        cycle_count++;
        elapsed_s++;
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SDCARD_WRITE_PERIOD_MS));
    }
}


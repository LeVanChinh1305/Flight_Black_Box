#include "task_gps.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define GPS_SIMULATE
#ifdef GPS_SIMULATE
#define GPS_SIM_LAT_BASE 21.028511f
#define GPS_SIM_LON_BASE 105.834160f
#define GPS_SIM_ALT_M 15.0f
#define GPS_SIM_SPEED_KMH 12.5f
#define GPS_SIM_SATELLITES 8
#define GPS_SIM_LAT_STEP 0.000050f
#define GPS_SIM_LON_STEP 0.000045f
#define GPS_SIM_PERIOD_MS 1000
#endif

static neo6m_data_t g_gps_data = {0};
static bool g_gps_data_valid = false;

bool GPS_GetLastData(neo6m_data_t *out) {
    if (out == NULL || g_mutex_gps_data == NULL) {
        return false;
    }

    bool ok = false;
    if (xSemaphoreTake(g_mutex_gps_data, pdMS_TO_TICKS(GPS_MUTEX_WAIT_MS)) == pdTRUE) {
        if (g_gps_data_valid) {
            *out = g_gps_data;
            ok = true;
        }
        xSemaphoreGive(g_mutex_gps_data);
    }
    return ok;
}

void TaskGPS(void *pvParameters) {
    (void)pvParameters;

#ifdef GPS_SIMULATE
    printf("[TaskGPS] Che do GIA LAP GPS (GPS_SIMULATE bat). Khong can phan cung.\n");
    static neo6m_data_t sim = {
        .latitude = GPS_SIM_LAT_BASE,
        .longitude = GPS_SIM_LON_BASE,
        .altitude_m = GPS_SIM_ALT_M,
        .fix_quality = NEO6M_FIX_GPS,
        .satellites = GPS_SIM_SATELLITES,
        .hdop = 1.2f,
        .speed_knots = GPS_SIM_SPEED_KMH / 1.852f,
        .speed_kmh = GPS_SIM_SPEED_KMH,
        .course_deg = 45.0f,
        .hour = 7,
        .minute = 0,
        .second = 0,
        .day = 9,
        .month = 7,
        .year = 2025,
        .is_valid = true,
    };

    TickType_t lastWake = xTaskGetTickCount();
    for (;;) {
        sim.second++;
        if (sim.second >= 60) {
            sim.second = 0;
            sim.minute++;
        }
        if (sim.minute >= 60) {
            sim.minute = 0;
            sim.hour++;
        }
        if (sim.hour >= 24) {
            sim.hour = 0;
        }

        sim.latitude += GPS_SIM_LAT_STEP;
        sim.longitude += GPS_SIM_LON_STEP;

        if (xSemaphoreTake(g_mutex_gps_data, pdMS_TO_TICKS(GPS_MUTEX_WAIT_MS)) == pdTRUE) {
            g_gps_data = sim;
            g_gps_data_valid = true;
            xSemaphoreGive(g_mutex_gps_data);
        }

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(GPS_SIM_PERIOD_MS));
    }
#else
    printf("[TaskGPS] Khoi tao NEO-6M...\n");
    neo6m_dev_t gps_dev;
    while (NEO6M_Init(&gps_dev, NEO6M_UART_PORT, NEO6M_PIN_TX, NEO6M_PIN_RX,
                      NEO6M_BAUDRATE) != NEO6M_OK) {
        printf("[TaskGPS] Khoi tao NEO-6M that bai, thu lai...\n");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    printf("[TaskGPS] Khoi tao NEO-6M thanh cong.\n");

    for (;;) {
        static neo6m_data_t tmp = {0};
        TickType_t t_start = xTaskGetTickCount();

        if (NEO6M_ReadLine(&gps_dev) == NEO6M_OK) {
            bool parsed = false;

            if (strncmp(gps_dev.nmea_buf, NEO6M_NMEA_GGA, 6) == 0) {
                if (NEO6M_ParseGGA(gps_dev.nmea_buf, &tmp) == NEO6M_OK)
                    parsed = true;
            } else if (strncmp(gps_dev.nmea_buf, NEO6M_NMEA_RMC, 6) == 0) {
                if (NEO6M_ParseRMC(gps_dev.nmea_buf, &tmp) == NEO6M_OK)
                    parsed = true;
            }

            if (parsed) {
                if (xSemaphoreTake(g_mutex_gps_data, pdMS_TO_TICKS(GPS_MUTEX_WAIT_MS)) == pdTRUE) {
                    g_gps_data = tmp;
                    g_gps_data_valid = true;
                    xSemaphoreGive(g_mutex_gps_data);
                }
            }
        }
    }
#endif
}

/**
 * main.cpp - Flight Black Box (Hop Den May Bay)
 *
 * Kien truc:
 *   - main() CHI lam 2 viec: (1) khoi tao phan cung nen tang (stdio...),
 *     (2) tao cac task FreeRTOS roi goi vTaskStartScheduler().
 *     Khong duoc viet logic doc cam bien / xu ly nghiep vu truc tiep trong
 * main().
 *   - Moi subsystem (BMI160, TFT, GPS, MQTT, buzzer...) la 1 task rieng,
 *     dinh nghia trong file/section rieng ben duoi de de mo rong.
 *   - Tai nguyen dung chung (SPI1 cho TFT+SD, UART cho GPS/SIM...) se dung
 *     Mutex / Queue / StreamBuffer, khai bao la bien global "handle" va
 *     truyen vao tung task qua tham so pvParameters khi tao task.
 *
 * Cach them 1 task moi:
 *   1. Viet ham "static void TaskXxx(void *pvParameters) { ... for(;;) {...} }"
 *      trong section TASK DEFINITIONS ben duoi.
 *   2. Goi xTaskCreate(...) cho task do trong ham main().
 */

#include "FreeRTOS.h"
#include "pico/stdlib.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include <stdio.h>


#include "algorithm_bmi160.h"
#include "bmi160.h"
#include "my_mqtt.h"
#include "neo6m.h"
#include "sim7680.h"


// =====================================================================
// ================        CAU HINH CHUNG TOAN CUC        ==============
// Uu tien task: so cang lon cang uu tien cao. Danh sach nay se mo rong
// dan khi them task moi de tien theo doi tuong quan uu tien giua cac task.
#define PRIORITY_TASK_BMI160 (tskIDLE_PRIORITY + 2)
#define PRIORITY_TASK_GPS (tskIDLE_PRIORITY + 2)
// MQTT/mang uu tien THAP hon cam bien: cac lenh AT co the mat vai giay,
// khong nen tranh CPU voi TaskBMI160/TaskGPS can do dung ky lay mau.
#define PRIORITY_TASK_MQTT (tskIDLE_PRIORITY + 1)

// =====================================================================
// ================     TAI NGUYEN DUNG CHUNG (mutex...)   ==============
// Vi du: SemaphoreHandle_t g_mutex_spi1 = NULL; // dung chung cho TFT + SD
// Khoi tao trong main() truoc khi tao cac task can dung no.
static SemaphoreHandle_t g_mutex_gps_data = NULL;
static SemaphoreHandle_t g_mutex_bmi_data = NULL;

// =====================================================================
// ================          TASK: BMI160 (IMU)            ==============
#define BMI160_TASK_STACK_SIZE 1024 // word (4 byte/word tren RP2040)
#define BMI160_READ_PERIOD_MS 200   // chu ky doc FIFO

// buffer FIFO dung rieng cho task nay (~200ms du lieu o 100Hz: ~20 frame * 12
// byte = 240 byte)
#define BMI160_FIFO_SCRATCH_SIZE 256
#define BMI160_FIFO_MAX_FRAMES 32

#define BMI160_MUTEX_WAIT_MS                                                   \
  50 // thoi gian toi da cho mutex khi cap nhat/doc du lieu

// Mau IMU moi nhat, dung chung cho toan he thong (bao ve boi g_mutex_bmi_data).
// Luu o don vi vat ly (g, do/giay) da qua algorithm_bmi160, khong phai raw LSB,
// de cac task tieu thu (blackbox log, MQTT...) dung thang khong can tu quy doi.
// Chi luu lai frame CUOI CUNG cua moi lan doc FIFO (~200ms/lan) - du cho muc
// dich log/telemetry; neu can toan bo frame cho tinh toan (vd. tich phan van
// toc) thi task tieu thu nen doc truc tiep tu 1 task rieng, khong qua bien nay.
static BMI160_Physical_t g_bmi_data;
static bool g_bmi_data_valid =
    false; // true ke tu lan doc FIFO co it nhat 1 frame

// Ham cong khai cho cac task khac (MQTT, Blackbox log...) lay mau IMU moi nhat
// (da o don vi vat ly). Tra ve true neu lay duoc du lieu.
bool BMI160_GetLastData(BMI160_Physical_t *out) {
  if (out == NULL || g_mutex_bmi_data == NULL)
    return false;

  bool ok = false;
  if (xSemaphoreTake(g_mutex_bmi_data, pdMS_TO_TICKS(BMI160_MUTEX_WAIT_MS)) ==
      pdTRUE) {
    if (g_bmi_data_valid) {
      *out = g_bmi_data;
      ok = true;
    }
    xSemaphoreGive(g_mutex_bmi_data);
  }
  return ok;
}

static void TaskBMI160(void *pvParameters) {
  (void)pvParameters;

  bmi_dev_t bmi;
  BMI160_Config_t config = {
      .accel_range = BMI_ACC_RANGE_4G,
      .gyro_range = BMI_GYR_RANGE_500DPS,
      .accel_odr = BMI_ACC_CONFIG_DEFAULT, // 100Hz
      .gyro_odr = BMI_GYR_CONFIG_DEFAULT,  // 100Hz
  };

  // khoi tao cam bien, neu loi thi thu lai moi 500ms (khong lam chet ca he
  // thong)
  while (BMI160_Init(&bmi, BMI160_I2C_PORT, BMI160_I2C_ADDR, &config) !=
         BMI_OK) {
    printf("[TaskBMI160] Khoi tao BMI160 that bai, thu lai...\n");
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  // cau hinh FIFO: bat acc + gyro, header mode de de giai ma
  BMI160_FIFO_Config_t fifo_cfg = {
      .acc_en = true,
      .gyr_en = true,
      .header_en = true,
      .time_en = false,
      .watermark = BMI_FIFO_WATERMARK_DEFAULT,
  };
  BMI160_FIFO_Config(&bmi, &fifo_cfg);

  uint8_t scratch_buf[BMI160_FIFO_SCRATCH_SIZE];
  BMI160_FIFO_Frame_t frames[BMI160_FIFO_MAX_FRAMES];
  BMI160_FIFO_Result_t result;

  // vong lap task: doc FIFO deu dan theo chu ky co dinh, khong bi troi (drift)
  // theo thoi gian xu ly - quan trong khi sau nay chay chung voi nhieu task
  // khac.
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    BMI_Status status =
        BMI160_FIFO_ReadAndParse(&bmi, scratch_buf, BMI160_FIFO_SCRATCH_SIZE,
                                 frames, BMI160_FIFO_MAX_FRAMES, &result);

    if (status == BMI_OK && result.frame_count > 0) {
      BMI160_FIFO_Frame_t *last = &frames[result.frame_count - 1];

      // Quy doi frame cuoi cung sang don vi vat ly (g, do/giay) truoc khi
      // dung/log - raw LSB khong co y nghia neu khong biet range dang cau hinh.
      BMI160_Physical_t phys;
      BMI_Status conv_status = BMI160_Algo_ConvertFrame(
          last, bmi.config.accel_range, bmi.config.gyro_range, &phys);

      if (conv_status == BMI_OK) {
        // Cap nhat mau IMU moi nhat cho cac task khac (mirror pattern cua GPS)
        if (xSemaphoreTake(g_mutex_bmi_data,
                           pdMS_TO_TICKS(BMI160_MUTEX_WAIT_MS)) == pdTRUE) {
          g_bmi_data = phys;
          g_bmi_data_valid = true;
          xSemaphoreGive(g_mutex_bmi_data);
        }

        printf("[TaskBMI160] frames=%u  Acc(%.3f,%.3f,%.3f)g |%.3f|g  "
               "Gyro(%.2f,%.2f,%.2f)dps\n",
               result.frame_count, phys.acc_x_g, phys.acc_y_g, phys.acc_z_g,
               phys.acc_magnitude_g, phys.gyr_x_dps, phys.gyr_y_dps,
               phys.gyr_z_dps);
      } else {
        // Chi xay ra neu range cau hinh sai ma khong khop bang tra cuu -
        // loi cau hinh, khong phai loi doc cam bien.
        printf("[TaskBMI160] Loi: range accel/gyro khong hop le, khong the quy "
               "doi don vi!\n");
      }
    } else if (status != BMI_OK) {
      printf("[TaskBMI160] Loi doc FIFO!\n");
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(BMI160_READ_PERIOD_MS));
  }
}

// =====================================================================
// ================          TASK: GPS                    ==============
#define GPS_TASK_STACK_SIZE 1024 // word (4 byte/word tren RP2040)
#define GPS_MUTEX_WAIT_MS                                                      \
  50 // thoi gian toi da cho mutex khi cap nhat/doc du lieu

// Ban sao du lieu GPS moi nhat, dung chung cho toan he thong (bao ve boi
// g_mutex_gps_data).
static neo6m_data_t g_gps_data;

// Ham cong khai cho cac task khac (MQTT, Blackbox log, TFT...) lay du lieu GPS
// moi nhat. Tra ve true neu lay duoc du lieu (co the vua fix vua mat fix, kiem
// tra them out->is_valid).
bool GPS_GetLastData(neo6m_data_t *out) {
  if (out == NULL || g_mutex_gps_data == NULL)
    return false;

  bool ok = false;
  if (xSemaphoreTake(g_mutex_gps_data, pdMS_TO_TICKS(GPS_MUTEX_WAIT_MS)) ==
      pdTRUE) {
    *out = g_gps_data;
    xSemaphoreGive(g_mutex_gps_data);
    ok = true;
  }
  return ok;
}
static void TaskGPS(void *pvParameters) {
  (void)pvParameters;

  neo6m_dev_t gps_dev;

  // khoi tao UART cho GPS, neu loi thi thu lai moi 500ms (khong lam chet ca he
  // thong)
  while (NEO6M_Init(&gps_dev, NEO6M_UART_PORT, NEO6M_PIN_TX, NEO6M_PIN_RX,
                    NEO6M_BAUDRATE) != NEO6M_OK) {
    printf("[TaskGPS] Khoi tao NEO-6M that bai, thu lai...\n");
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  // vong lap task: NEO6M_Update tu cho (block) ben trong toi khi doc duoc 1
  // dong NMEA hoan chinh hoac het timeout (NEO6M_UART_TIMEOUT_MS), nen khong
  // can vTaskDelay them o day - ban than ham doc da nhuong CPU dung ky
  // (vTaskDelay trong NEO6M_ReadLine) cho cac task khac trong luc cho UART.
  for (;;) {
    neo6m_data_t tmp = {0};
    NEO6M_Status status = NEO6M_Update(&gps_dev, &tmp);

    if (status == NEO6M_OK) {
      // Chi ghi vao bien dung chung khi thuc su co ban tin GGA/RMC da parse
      // (status == NEO6M_UNKNOWN nghia la doc duoc dong khac, khong cap nhat
      // tmp)
      if (xSemaphoreTake(g_mutex_gps_data, pdMS_TO_TICKS(GPS_MUTEX_WAIT_MS)) ==
          pdTRUE) {
        g_gps_data = tmp;
        xSemaphoreGive(g_mutex_gps_data);
      }

      if (tmp.is_valid) {
        printf("[TaskGPS] Fix OK - Lat=%.6f Lon=%.6f Alt=%.1fm Sat=%u "
               "Speed=%.1fkm/h\n",
               tmp.latitude, tmp.longitude, tmp.altitude_m, tmp.satellites,
               tmp.speed_kmh);
      } else {
        printf("[TaskGPS] Chua co fix (sat=%u)\n", tmp.satellites);
      }
    } else if (status == NEO6M_ERROR) {
      printf("[TaskGPS] Loi doc/hoac checksum sai, bo qua dong nay\n");
    }
    // status == NEO6M_UNKNOWN: dong NMEA khac (GPVTG, GPGSA...), bo qua im lang
  }
}

// =====================================================================
// ================          TASK: MQTT (SIM7680)          ==============
#define MQTT_TASK_STACK_SIZE                                                   \
  2048 // word - lon hon cac task khac vi AT/JSON buffer nhieu
#define MQTT_SIM_READY_TIMEOUT_MS 15000 // cho module SIM phan hoi AT co ban
#define MQTT_NET_REG_TIMEOUT_MS 30000   // cho dang ky mang di dong
#define MQTT_NET_POLL_PERIOD_MS 2000    // chu ky poll trang thai mang
#define MQTT_PUBLISH_PERIOD_MS 5000     // chu ky publish telemetry
#define MQTT_PROCESS_POLL_PERIOD_MS 100 // chu ky quet URC/lenh den tu broker

static void TaskMQTT(void *pvParameters) {
  (void)pvParameters;

  sim7680_init();

  for (;;) {
    // ---- 1) Cho module SIM phan hoi lenh AT co ban ----
    printf("[TaskMQTT] Dang cho module SIM khoi dong...\n");
    if (!sim7680_wait_ready(MQTT_SIM_READY_TIMEOUT_MS)) {
      printf("[TaskMQTT] Module SIM khong phan hoi, thu lai...\n");
      vTaskDelay(pdMS_TO_TICKS(2000));
      continue;
    }

    // ---- 2) Cho dang ky mang di dong (status 1 = home, 5 = roaming) ----
    printf("[TaskMQTT] Dang cho dang ky mang...\n");
    bool net_registered = false;
    TickType_t net_deadline =
        xTaskGetTickCount() + pdMS_TO_TICKS(MQTT_NET_REG_TIMEOUT_MS);
    while (xTaskGetTickCount() < net_deadline) {
      int net_status = 0;
      if (sim7680_get_network_status(&net_status) &&
          (net_status == 1 || net_status == 5)) {
        net_registered = true;
        break;
      }
      vTaskDelay(pdMS_TO_TICKS(MQTT_NET_POLL_PERIOD_MS));
    }
    if (!net_registered) {
      printf("[TaskMQTT] Khong dang ky duoc mang, thu lai tu dau...\n");
      vTaskDelay(pdMS_TO_TICKS(2000));
      continue;
    }
    printf("[TaskMQTT] Da dang ky mang thanh cong.\n");

    // ---- 3) Khoi tao + ket noi MQTT broker ----
    if (!mqtt_init() || !mqtt_connect()) {
      printf("[TaskMQTT] Khong ket noi duoc MQTT broker, thu lai tu dau...\n");
      vTaskDelay(pdMS_TO_TICKS(3000));
      continue;
    }

    mqtt_subscribe(MQTT_TOPIC_COMMAND);

    // ---- 4) Vong lap chinh: quet lenh den + publish telemetry dinh ky ----
    // Quet mqtt_process() moi MQTT_PROCESS_POLL_PERIOD_MS (nhanh, non-blocking)
    // va publish moi MQTT_PUBLISH_PERIOD_MS (dem so vong quet thay vi goi
    // vTaskDelay dai roi moi xu ly URC - tranh bo lot lenh tu broker).
    const uint32_t publish_every_n_polls =
        MQTT_PUBLISH_PERIOD_MS / MQTT_PROCESS_POLL_PERIOD_MS;
    uint32_t poll_count = 0;

    while (true) {
      mqtt_process();

      if (++poll_count >= publish_every_n_polls) {
        poll_count = 0;

        neo6m_data_t gps = {0};
        BMI160_Physical_t imu = {0};
        bool has_gps = GPS_GetLastData(&gps);
        bool has_imu = BMI160_GetLastData(&imu);

        char payload[256];
        snprintf(payload, sizeof(payload),
                 "{\"gps_fix\":%s,\"lat\":%.6f,\"lon\":%.6f,\"alt_m\":%.1f,"
                 "\"sat\":%u,"
                 "\"speed_kmh\":%.1f,\"acc_g\":[%.3f,%.3f,%.3f],\"gyro_dps\":[%"
                 ".2f,%.2f,%.2f]}",
                 (has_gps && gps.is_valid) ? "true" : "false", gps.latitude,
                 gps.longitude, gps.altitude_m, gps.satellites, gps.speed_kmh,
                 has_imu ? imu.acc_x_g : 0.0f, has_imu ? imu.acc_y_g : 0.0f,
                 has_imu ? imu.acc_z_g : 0.0f, has_imu ? imu.gyr_x_dps : 0.0f,
                 has_imu ? imu.gyr_y_dps : 0.0f,
                 has_imu ? imu.gyr_z_dps : 0.0f);

        if (mqtt_publish(MQTT_TOPIC_TELEMETRY, payload, false)) {
          printf("[TaskMQTT] Published: %s\n", payload);
        } else {
          // Publish that bai (vd. mat song) -> thoat vong lap, quay lai
          // tu dau de reconnect thay vi tiep tuc publish vao ket noi chet.
          printf("[TaskMQTT] Publish that bai, ket noi lai tu dau...\n");
          break;
        }
      }

      vTaskDelay(pdMS_TO_TICKS(MQTT_PROCESS_POLL_PERIOD_MS));
    }
  }
}

// =====================================================================
// ================      TASK MOI SE DUOC THEM O DAY        =============
// =====================================================================
// static void TaskTFT(void *pvParameters)   { ... }
// static void TaskBuzzer(void *pvParameters){ ... }

// =====================================================================
// ================   HOOK BAT BUOC CUA FREERTOS            =============
// =====================================================================
// Duoc goi tu dong khi configCHECK_FOR_STACK_OVERFLOW != 0 (dang bat trong
// FreeRTOSConfig.h) va phat hien 1 task dung vuot qua stack da cap.
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                              char *pcTaskName) {
  (void)xTask;
  printf("!!! STACK OVERFLOW o task: %s !!!\n", pcTaskName);
  taskDISABLE_INTERRUPTS();
  for (;;) {
    tight_loop_contents();
  }
}

// =====================================================================
// ================                MAIN                      =============
// =====================================================================

int main() {
  stdio_init_all();
  sleep_ms(2000); // cho mo cong USB-serial de khong mat log dau tien (bo neu
                  // dung UART thuong)

  // Khoi tao tai nguyen dung chung TRUOC khi tao task su dung no
  g_mutex_gps_data = xSemaphoreCreateMutex();
  g_mutex_bmi_data = xSemaphoreCreateMutex();
  if (g_mutex_gps_data == NULL || g_mutex_bmi_data == NULL) {
    printf("[main] Khong the tao mutex GPS/BMI160, dung he thong!\n");
    while (true) {
      tight_loop_contents();
    }
  } else {
    printf("[main] Mutex GPS/BMI160 da san sang.\n");
  }
  printf("=== Flight Black Box - Bat dau chuong trinh ===\n");

  // ---- khoi tao tai nguyen dung chung truoc khi tao task (neu co) ----
  // vi du: g_mutex_spi1 = xSemaphoreCreateMutex();

  // ---- tao cac task ----
  BaseType_t ok_BMI160 =
      xTaskCreate(TaskBMI160,             // ham task
                  "TaskBMI160",           // ten task (hien trong debug/log)
                  BMI160_TASK_STACK_SIZE, // kich thuoc stack (word)
                  NULL,                   // tham so truyen vao task
                  PRIORITY_TASK_BMI160,   // do uu tien
                  NULL                    // khong can luu handle
      );
  BaseType_t ok_GPS = xTaskCreate(TaskGPS, "GPS", GPS_TASK_STACK_SIZE, NULL,
                                  PRIORITY_TASK_GPS, NULL);

  if (ok_BMI160 != pdPASS || ok_GPS != pdPASS) {
    printf("Loi: khong the tao task!\n");
    while (true) {
      tight_loop_contents();
    }
  }

  // khoi dong scheduler, tu day FreeRTOS quan ly toan bo vong lap
  vTaskStartScheduler();

  // khong bao gio chay toi day neu scheduler khoi dong thanh cong
  while (true) {
    tight_loop_contents();
  }
  return 0;
}
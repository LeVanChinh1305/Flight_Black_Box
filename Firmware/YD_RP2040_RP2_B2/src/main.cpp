#include "FreeRTOS.h"
#include "pico/stdlib.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include <stdio.h>

#include "algorithm_bmi160.h"
#include "bmi160.h"
#include "buzzer.h"
#include "ff.h" // FatFs: dung de tao/ghi file that (.csv) tren the SD
#include "my_mqtt.h"
#include "neo6m.h"
#include "sdcard.h" // can cho kieu SD_Status (SD_OK / SD_ERROR) dung trong TaskSDCard
#include "sim7680.h"
#include <string.h>

#include "tft.h"
#include "ui.h"
#include "xpt2046.h"

// ================Định nghĩa mức độ ưu tiên (Priority) cho các Task trong
// FreeRTOS==============
#define PRIORITY_TASK_BMI160 (tskIDLE_PRIORITY + 3)
#define PRIORITY_TASK_GPS (tskIDLE_PRIORITY + 2)

// ================     Tài nguyên dùng chung (mutex...)   ==============
static SemaphoreHandle_t g_mutex_gps_data =
    NULL; // mutex bảo vệ dữ liệu GPS (g_gps_data) khi được truy cập bởi nhiều
          // task
static SemaphoreHandle_t g_mutex_bmi_data =
    NULL; // mutex bảo vệ dữ liệu BMI160 (g_bmi_data) khi được truy cập bởi
          // nhiều task
static SemaphoreHandle_t g_mutex_spi0 =
    NULL; // mutex bảo vệ bus SPI0 (SDCard + XPT2046)

// ================     BUZZER: Queue & lệnh điều khiển     ==============
// Giao tiếp bằng Queue để task khác gửi lệnh mà không bị chặn luồng.
typedef enum {
  BUZZER_CMD_BOOT_OK = 0,    // 2 bíp ngắn lúc khởi động thành công
  BUZZER_CMD_BOOT_ERROR = 1, // 1 bíp dài báo lỗi nghiêm trọng
  BUZZER_CMD_BEEP_SHORT = 2, // 1 bíp ngắn (100ms) - xác nhận hành động
  BUZZER_CMD_BEEP_LONG = 3,  // 1 bíp dài (500ms)  - cảnh báo nhẹ
  BUZZER_CMD_ALARM = 4,      // bíp liên tục 3s/lần - chế độ định vị
  BUZZER_CMD_ALARM_STOP = 5, // dừng chế độ định vị
} BuzzerCmd_t;

static QueueHandle_t g_buzzer_queue = NULL; // Queue gửi lệnh tới TaskBuzzer

// Gửi lệnh tới TaskBuzzer từ bất kỳ task nào (non-blocking)
// Trả về true nếu gửi thành công, false nếu queue đầy (bỏ qua lệnh)
static inline bool Buzzer_SendCmd(BuzzerCmd_t cmd) {
  if (g_buzzer_queue == NULL)
    return false;
  return xQueueSend(g_buzzer_queue, &cmd, 0) == pdTRUE;
}

// ================          TASK: BMI160 (IMU)            ==============
#define BMI160_TASK_STACK_SIZE                                                 \
  1024 // Cấp phát kích thước vùng nhớ Stack cho task
#define BMI160_READ_PERIOD_MS                                                  \
  200 // Chu kỳ chạy của task. Cứ mỗi 200ms, task sẽ thức dậy một lần để đọc bộ
      // nhớ đệm FIFO của cảm biến.
#define BMI160_FIFO_SCRATCH_SIZE                                               \
  256 // Kích thước mảng đệm tạm thời (bằng byte) dùng để chứa dữ liệu thô đọc
      // ra từ thanh ghi FIFO
#define BMI160_FIFO_MAX_FRAMES                                                 \
  32 // số frame tối đa có thể đọc trong một lần đọc FIFO (tăng nếu muốn đọc
     // nhiều frame hơn)
#define BMI160_MUTEX_WAIT_MS                                                   \
  50 // Thời gian tối đa (50ms) mà một hàm chấp nhận "đứng đợi" để lấy được
     // Mutex

static BMI160_Physical_t
    g_bmi_data; // biến toàn cục lưu trữ dữ liệu vật lý (đơn vị g, dps) của cảm
                // biến BMI160, được bảo vệ bởi mutex g_mutex_bmi_data
static bool g_bmi_data_valid =
    false; // true kể từ khi có dữ liệu BMI160 hợp lệ đầu tiên, false nếu chưa
           // có dữ liệu hợp lệ , dùng để kiểm tra xem dữ liệu BMI160 có hợp lệ
           // hay không trước khi sử dụng
bool BMI160_GetLastData(
    BMI160_Physical_t
        *out) { // hàm lấy dữ liệu BMI160 mới nhất, trả về true nếu có dữ liệu
                // hợp lệ, false nếu không có dữ liệu hợp lệ
  if (out == NULL || g_mutex_bmi_data == NULL)
    return false; // nếu con trỏ đầu ra là NULL hoặc mutex chưa được khởi tạo,
                  // trả về false

  bool ok = false;
  if (xSemaphoreTake(g_mutex_bmi_data, pdMS_TO_TICKS(BMI160_MUTEX_WAIT_MS)) ==
      pdTRUE) { // nếu lấy được mutex thành công trong thời gian chờ, thực hiện
                // truy cập dữ liệu BMI160
    if (g_bmi_data_valid) { // nếu dữ liệu BMI160 hợp lệ, sao chép dữ liệu vào
                            // con trỏ đầu ra và đặt ok = true
      *out = g_bmi_data;
      ok = true;
    }
    xSemaphoreGive(g_mutex_bmi_data); // giải phóng mutex sau khi truy cập xong
  }
  return ok;
}
static void TaskBMI160(void *pvParameters) {
  (void)pvParameters; // tránh cảnh báo biến không được sử dụng
  printf("[TaskBMI160] Khoi tao BMI160...\n");

  // B1:  Khởi tạo cảm biến BMI160 với cấu hình mặc định, nếu khởi tạo thất bại,
  // in ra thông báo lỗi và lặp lại sau 500ms
  bmi_dev_t bmi; // khai báo biến bmi với kiểu dữ liệu bmi_dev_t
  BMI160_Config_t config = {
      .accel_range = BMI_ACC_RANGE_4G,     // Dải đo gia tốc: ±4g
      .gyro_range = BMI_GYR_RANGE_500DPS,  // Dải đo vận tốc góc: ±500 độ/giây
      .accel_odr = BMI_ACC_CONFIG_DEFAULT, // Tần số lấy mẫu gia tốc: 100Hz
      .gyro_odr =
          BMI_GYR_CONFIG_DEFAULT, // Tần số lấy mẫu con quay hồi chuyển: 100Hz
  };
  while (BMI160_Init(&bmi, BMI160_I2C_PORT, BMI160_I2C_ADDR, &config) !=
         BMI_OK) { // nếu khởi tạo BMI160 thất bại, in ra thông báo lỗi và lặp
                   // lại sau 500ms
    printf("[TaskBMI160] Khoi tao BMI160 that bai, thu lai...\n");
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  // B2: Cấu hình chế độ FIFO của BMI160 để đọc dữ liệu gia tốc và con quay, bật
  // chế độ header để dễ dàng giải mã dữ liệu, không sử dụng ngưỡng cảnh báo
  // (watermark = 0), chế độ ghi đè khi FIFO đầy, bật cảnh báo tràn dữ liệu
  // (overrun) cấu hình FIFO: bat acc + gyro, header mode de de giai ma
  BMI160_FIFO_Config_t fifo_cfg = {
      .acc_en = true,
      .gyr_en = true,
      .header_en = true,
      .time_en = false,
      .watermark = 0, // watermark = 0: không sử dụng ngưỡng cảnh báo, đọc FIFO
                      // theo chu kỳ định sẵn
      .fifo_mode = BMI160_FIFO_MODE_OVERWRITE, // chế độ ghi đè, khi FIFO đầy sẽ
                                               // ghi đè dữ liệu cũ
      .overrun_en = true,
  };
  while (BMI160_FIFO_Config(&bmi, &fifo_cfg) != BMI_OK) { // cấu hình FIFO
    printf("[TaskBMI160] Cau hinh FIFO that bai, thu lai...\n");
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  // B3: Khởi tạo bộ nhớ đệm tạm thời scratch_buf để đọc dữ liệu thô từ FIFO,
  // khởi tạo mảng frames để lưu trữ các frame dữ liệu đã phân tích, khởi tạo
  // biến result để lưu trữ kết quả phân tích dữ liệu FIFO, lấy thời gian hiện
  // tại lastWake để tính toán chu kỳ đọc FIFO
  uint8_t scratch_buf[BMI160_FIFO_SCRATCH_SIZE]; // khởi tạo bộ nhớ đệm tạm thời
  BMI160_FIFO_Frame_t frames[BMI160_FIFO_MAX_FRAMES]; // khởi tạo bộ nhớ đệm để
                                                      // lưu trữ các frame
  BMI160_FIFO_Result_t result; // khởi tạo cấu trúc để lưu trữ kết quả
  TickType_t lastWake = xTaskGetTickCount(); // lấy thời gian hiện tại
  for (;;) { // vòng lặp vô hạn để đọc dữ liệu BMI160
    // đọc dữ liệu FIFO và phân tích dữ liệu thành các frame, lưu trữ kết quả
    // vào biến result. Nếu có lỗi trong quá trình đọc FIFO, in ra thông báo
    // lỗi.
    BMI_Status status =
        BMI160_FIFO_ReadAndParse(&bmi, scratch_buf, BMI160_FIFO_SCRATCH_SIZE,
                                 frames, BMI160_FIFO_MAX_FRAMES, &result);

    // Nếu đọc và phân tích dữ liệu FIFO thành công và có ít nhất một frame dữ
    // liệu, thực hiện chuyển đổi frame cuối cùng sang đơn vị vật lý (g, dps) và
    // cập nhật dữ liệu BMI160 mới nhất cho các task khác thông qua mutex. Nếu
    // có lỗi trong quá trình đọc FIFO hoặc chuyển đổi đơn vị, in ra thông báo
    // lỗi.
    if (status == BMI_OK && result.frame_count > 0) {
      BMI160_FIFO_Frame_t *last =
          &frames[result.frame_count -
                  1];         // lấy frame cuối cùng trong mảng frames
      BMI160_Physical_t phys; // khai báo biến phys để lưu trữ dữ liệu vật lý
                              // sau khi chuyển đổi
      if (BMI160_Algo_ConvertFrame(last, bmi.config.accel_range,
                                   bmi.config.gyro_range, &phys) == BMI_OK) {
        if (xSemaphoreTake(g_mutex_bmi_data,
                           pdMS_TO_TICKS(BMI160_MUTEX_WAIT_MS)) == pdTRUE) {
          g_bmi_data = phys; // cập nhật dữ liệu vật lý mới nhất vào biến toàn
                             // cục g_bmi_data
          g_bmi_data_valid = true; // đánh dấu dữ liệu BMI160 là hợp lệ kể từ
                                   // khi có frame đầu tiên
          xSemaphoreGive(g_mutex_bmi_data);
        }

        TickType_t now = xTaskGetTickCount();
        // printf("[TaskBMI160] thoi gian hien tai = %u ms, ", (unsigned
        // int)(now * portTICK_PERIOD_MS)); printf("[TaskBMI160] Thoi gian doc
        // FIFO: %u ms\n", (unsigned int)(now - lastWake) * portTICK_PERIOD_MS);

        // printf(" [TaskBMI160] frames=%u Acc(%.3f,%.3f,%.3f)g |%.3f|g "
        //        "Gyro(%.2f,%.2f,%.2f)dps\n",
        //        result.frame_count, phys.acc_x_g, phys.acc_y_g, phys.acc_z_g,
        //        phys.acc_magnitude_g, phys.gyr_x_dps, phys.gyr_y_dps,
        //        phys.gyr_z_dps);
      }
    } else if (status != BMI_OK) {
      printf("[TaskBMI160] Loi doc FIFO!\n");
    }
    vTaskDelayUntil(
        &lastWake,
        pdMS_TO_TICKS(BMI160_READ_PERIOD_MS)); // delay cho đến khi chu kỳ tiếp
                                               // theo bắt đầu
    // lastWake : là thời điểm mà task được đánh thức lần cuối cùng, được cập
    // nhật bởi hàm vTaskDelayUntil() để đảm bảo rằng task sẽ được đánh thức
    // đúng chu kỳ định sẵn.
  }
}

// ================          TASK: GPS                    ==============
#define GPS_TASK_STACK_SIZE 1024 // word (4 byte/word tren RP2040)
#define GPS_MUTEX_WAIT_MS                                                      \
  50 // thoi gian toi da cho mutex khi cap nhat/doc du lieu

// ----------------------------------------------------------------
// BẬT/TẮT chế độ giả lập GPS (không cần phần cứng, không cần tín hiệu vệ tinh)
// Khi ở trong nhà hoặc muốn test: giữ nguyên dòng dưới
// Khi dùng phần cứng thật ngoài trời:  comment dòng dưới lại
// ----------------------------------------------------------------
#define GPS_SIMULATE

#ifdef GPS_SIMULATE
// Tọa độ gốc giả lập (Hà Nội - Hoàn Kiếm)
#define GPS_SIM_LAT_BASE 21.028511f
#define GPS_SIM_LON_BASE 105.834160f
#define GPS_SIM_ALT_M 15.0f
#define GPS_SIM_SPEED_KMH 12.5f
#define GPS_SIM_SATELLITES 8
// Mỗi chu kỳ vị trí dịch chuyển một lượng nhỏ để giả lập chuyển động
#define GPS_SIM_LAT_STEP 0.000050f // ~5.5 m mỗi giây về hướng Bắc
#define GPS_SIM_LON_STEP 0.000045f // ~4.5 m mỗi giây về hướng Đông
#define GPS_SIM_PERIOD_MS 1000 // Gửi dữ liệu mỗi 1 giây (đúng như module thật)
#endif                         // GPS_SIMULATE

static neo6m_data_t g_gps_data;
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

#ifdef GPS_SIMULATE
  // ---- Chế độ giả lập: không khởi tạo UART, không cần tín hiệu vệ tinh ----
  printf("[TaskGPS] Che do GIA LAP GPS (GPS_SIMULATE bat). Khong can phan "
         "cung.\n");

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
    // Cập nhật thời gian giả (tăng 1 giây mỗi chu kỳ)
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

    // Dịch chuyển tọa độ nhỏ để giả lập chuyển động
    sim.latitude += GPS_SIM_LAT_STEP;
    sim.longitude += GPS_SIM_LON_STEP;

    // Ghi vào biến toàn cục (bảo vệ bằng mutex)
    if (xSemaphoreTake(g_mutex_gps_data, pdMS_TO_TICKS(GPS_MUTEX_WAIT_MS)) ==
        pdTRUE) {
      g_gps_data = sim;
      xSemaphoreGive(g_mutex_gps_data);
    }

    TickType_t now = xTaskGetTickCount();
    // printf("[TaskGPS][SIM] t=%ums Fix OK - vido=%.6f kinhdo=%.6f docao=%.1fm "
    //        "sat=%u tocdo=%.1fkm/h %02u:%02u:%02u\n",
    //        (unsigned int)(now * portTICK_PERIOD_MS), sim.latitude,
    //        sim.longitude, sim.altitude_m, (unsigned int)sim.satellites,
    //        sim.speed_kmh, (unsigned int)sim.hour, (unsigned int)sim.minute,
    //        (unsigned int)sim.second);

    // Delay chính xác đến chu kỳ tiếp theo (1 giây)
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(GPS_SIM_PERIOD_MS));
  }

#else
  // ---- Chế độ thực: đọc dữ liệu từ module NEO-6M qua UART ----
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
      TickType_t t_read_done = xTaskGetTickCount();
      uint32_t read_ms = (t_read_done - t_start) * portTICK_PERIOD_MS;

      bool parsed = false;

      if (strncmp(gps_dev.nmea_buf, NEO6M_NMEA_GGA, 6) == 0) {
        if (NEO6M_ParseGGA(gps_dev.nmea_buf, &tmp) == NEO6M_OK)
          parsed = true;
      } else if (strncmp(gps_dev.nmea_buf, NEO6M_NMEA_RMC, 6) == 0) {
        if (NEO6M_ParseRMC(gps_dev.nmea_buf, &tmp) == NEO6M_OK)
          parsed = true;
      }

      TickType_t t_parse_done = xTaskGetTickCount();
      uint32_t parse_ms = (t_parse_done - t_read_done) * portTICK_PERIOD_MS;

      printf("[TaskGPS] doc=%ums parse=%ums [%s] -> %s\n",
             (unsigned int)read_ms, (unsigned int)parse_ms, gps_dev.nmea_buf,
             parsed ? "OK" : "BO QUA");

      if (parsed) {
        if (xSemaphoreTake(g_mutex_gps_data,
                           pdMS_TO_TICKS(GPS_MUTEX_WAIT_MS)) == pdTRUE) {
          g_gps_data = tmp;
          xSemaphoreGive(g_mutex_gps_data);
        }

        if (strncmp(gps_dev.nmea_buf, NEO6M_NMEA_RMC, 6) == 0) {
          TickType_t now_gps = xTaskGetTickCount();
          if (tmp.is_valid) {
            printf("[TaskGPS] t=%ums Fix OK - vido=%.6f kinhdo=%.6f "
                   "docao=%.1fm sovetinh=%u tocdo=%.1fkm/h\n",
                   (unsigned int)(now_gps * portTICK_PERIOD_MS), tmp.latitude,
                   tmp.longitude, tmp.altitude_m, tmp.satellites,
                   tmp.speed_kmh);
          } else {
            printf("[TaskGPS] t=%ums Chua co fix (sat=%u)\n",
                   (unsigned int)(now_gps * portTICK_PERIOD_MS),
                   tmp.satellites);
          }
        }
      }
    }
  }
#endif // GPS_SIMULATE
}

// ================          TASK: SD CARD                ==============
#define PRIORITY_TASK_SDCARD                                                   \
  (tskIDLE_PRIORITY + 1)            // Độ ưu tiên thấp hơn IMU một chút
#define SDCARD_TASK_STACK_SIZE 2048 // FatFs cần stack lớn hơn (khoảng 2KB)
#define SDCARD_WRITE_PERIOD_MS 1000 // Ghi log định kỳ mỗi 1 giây
#define SDCARD_ROTATE_EVERY_N_CYCLES                                           \
  60 // 60 chu kỳ (60 x 1s = 1 phút) -> tạo file mới
#define SDCARD_MOUNT_RETRY_MS 1000 // thời gian chờ giữa các lần thử mount lại

// FatFs chỉ hỗ trợ tên file kiểu 8.3 trong cấu hình hiện tại (FF_USE_LFN = 0
// trong ffconf.h), nên KHÔNG thể đặt tên dài kiểu
// "log_2026-07-16_14-35-00.csv". Ta nén thời gian vào 6 số:
// - Nếu GPS đã có fix hợp lệ: DDHHMM.CSV (ngày-giờ-phút UTC lấy từ vệ tinh)
// - Nếu GPS CHƯA có fix (vd mới bật máy, chưa bắt được vệ tinh): dùng số phút
// kể từ lúc
//   boot, tiền tố "B" để phân biệt với tên theo giờ GPS -> B000001.CSV,
//   B000002.CSV, ...
#define SDLOG_FILENAME_MAXLEN 13 // "8 ky tu" + "." + "3 ky tu" + '\0'

static FATFS g_fatfs;    // đối tượng filesystem của FatFs (bắt buộc phải "sống"
                         // suốt thời gian mount)
static FIL g_sdlog_file; // handle của file log đang mở
static bool g_sdlog_file_open = false;
static char g_sdlog_current_name[SDLOG_FILENAME_MAXLEN] = {
    0}; // ten file dang mo (de log khi dong)
static uint32_t g_sdlog_record_count =
    0; // so dong da ghi vao file HIEN TAI (reset khi mo file moi)

// Dựng tên file 8.3 dựa trên thời gian GPS (nếu có fix) hoặc số phút từ lúc
// khởi động (fallback)
static void SDLog_BuildFileName(char *out, size_t out_len,
                                const neo6m_data_t *gps, uint32_t uptime_min) {
  if (gps->is_valid) {
    snprintf(out, out_len, "%02u%02u%02u.CSV", (unsigned)gps->day,
             (unsigned)gps->hour, (unsigned)gps->minute);
  } else {
    snprintf(out, out_len, "B%06lu.CSV",
             (unsigned long)(uptime_min % 1000000UL));
  }
}

// Đóng file cũ (nếu có) và mở file mới để ghi, kèm dòng tiêu đề CSV.
// In log DEBUG ro rang o ca 2 thoi diem: luc dong file cu (tong ket) va luc mo
// file moi (xac nhan).
static SD_Status SDLog_OpenNewFile(const char *filename) {
  if (g_mutex_spi0)
    xSemaphoreTake(g_mutex_spi0, portMAX_DELAY);
  if (g_sdlog_file_open) {
    FSIZE_t size_before_close = f_size(&g_sdlog_file);
    f_close(&g_sdlog_file);
    g_sdlog_file_open = false;
    printf(
        "[TaskSDCard][DEBUG] Da dong file '%s': %lu dong du lieu, %lu byte.\n",
        g_sdlog_current_name, (unsigned long)g_sdlog_record_count,
        (unsigned long)size_before_close);
  }

  FRESULT fr = f_open(&g_sdlog_file, filename, FA_WRITE | FA_CREATE_ALWAYS);
  if (fr != FR_OK) {
    if (g_mutex_spi0)
      xSemaphoreGive(g_mutex_spi0);
    printf("[TaskSDCard][DEBUG] LOI f_open('%s') fr=%d -> KHONG mo duoc file "
           "moi!\n",
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

  printf("[TaskSDCard][DEBUG] >>> Da MO file log MOI: '%s' (header %u/%u byte, "
         "fw=%d)\n",
         filename, (unsigned)bw, (unsigned)(sizeof(header) - 1), fw);
  return SD_OK;
}

static void TaskSDCard(void *pvParameters) {
  (void)pvParameters;
  printf("[TaskSDCard] Khoi tao FatFs...\n");

  // B1: mount filesystem. f_mount() se goi disk_initialize() -> SDCARD_Init()
  // ben trong. Neu chua nhan the/loi phan cung, thu lai dinh ky thay vi treo
  // chuong trinh.
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

  uint32_t cycle_count =
      0; // dem so chu ky 1 giay da chay, dung de biet khi nao sang phut moi
  uint32_t elapsed_s = 0; // tong so giay ke tu luc task bat dau (dung lam cot
                          // "time_s" trong CSV)
  char filename[SDLOG_FILENAME_MAXLEN];

  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    // B2: lay du lieu MOI NHAT tu 2 task kia. Dung ham Getter co san (co mutex
    // ben trong), KHONG duoc truy cap truc tiep g_bmi_data / g_gps_data tu day
    // de tranh dua du lieu (race condition).
    BMI160_Physical_t bmi = {0};
    neo6m_data_t gps = {0};
    bool has_bmi = BMI160_GetLastData(&bmi);
    bool has_gps = GPS_GetLastData(&gps);

    // B3: cu moi 60 chu ky (~60 giay = 1 phut) thi dong file cu, tao file moi.
    // Dieu kien cycle_count == 0 dam bao file dau tien duoc tao ngay khi vao
    // vong lap.
    if (cycle_count % SDCARD_ROTATE_EVERY_N_CYCLES == 0) {
      printf("[TaskSDCard][DEBUG] Toi han xoay file (elapsed=%lus, phut thu "
             "%lu) -> tao file moi...\n",
             (unsigned long)elapsed_s, (unsigned long)(elapsed_s / 60));
      SDLog_BuildFileName(filename, sizeof(filename), &gps, elapsed_s / 60);
      SDLog_OpenNewFile(filename); // ham nay tu in log ca luc thanh cong lan
                                   // that bai (xem SDLog_OpenNewFile)
    }

    // B4: ghi 1 dong du lieu (moi giay 1 dong) vao file dang mo.
    if (g_sdlog_file_open) {
      char line[160];
      int len = snprintf(
          line, sizeof(line),
          "%lu,%.3f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.6f,%.6f,%.1f,%.1f,%u,%"
          "d\r\n",
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
        // f_sync ghi thang xuong the ngay (khong doi buffer day), giup du lieu
        // khong bi mat neu mat dien dot ngot -- danh doi la ton hao mon the hon
        // so voi ghi gom (batch).
        f_sync(&g_sdlog_file);
        g_sdlog_record_count++;
        if (g_mutex_spi0)
          xSemaphoreGive(g_mutex_spi0);
        // KHONG log moi giay o day nua (qua on ao) -- so dong ghi duoc se duoc
        // tong ket 1 lan khi dong file, xem SDLog_OpenNewFile().
      }
    }

    cycle_count++;
    elapsed_s++;
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SDCARD_WRITE_PERIOD_MS));
  }
}

// ================          TASK: MQTT                   ==============
#define PRIORITY_TASK_MQTT (tskIDLE_PRIORITY + 1)
#define MQTT_TASK_STACK_SIZE 3072
#define MQTT_PUBLISH_PERIOD_MS 1000 // Gửi mỗi 1s

typedef enum {
  MQTT_STATE_INIT_SIM = 0,
  MQTT_STATE_CONNECT,
  MQTT_STATE_SUBSCRIBE,
  MQTT_STATE_RUNNING,
  MQTT_STATE_RECONNECT
} MQTT_State_t;

// Hàm callback khi nhận được tin nhắn MQTT (override hàm weak trong
// my_mqtt.cpp)
void mqtt_message_received(const char *topic, const char *payload) {
  printf("[main] MQTT_RX [%s]: %s\n", topic, payload);
  // Bíp 1 cái để báo hiệu có lệnh
  Buzzer_SendCmd(BUZZER_CMD_BEEP_SHORT);

  // Tương lai: Parse payload JSON/String để điều khiển hệ thống
}

static void TaskMQTT(void *pvParameters) {
  (void)pvParameters;
  MQTT_State_t state = MQTT_STATE_INIT_SIM;
  TickType_t last_pub_time = 0;

  for (;;) {
    switch (state) {
    case MQTT_STATE_INIT_SIM: { // Khởi tạo SIM và chờ SIM sẵn sàng hhh
      printf("[TaskMQTT] Trang thai: INIT_SIM\n");
      sim7680_init();
      printf("[TaskMQTT] Cho SIM ready...\n");
      while (!sim7680_wait_ready(10000)) {
        printf("[TaskMQTT] SIM chua ready, thu lai...\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
      }

      bool sim_ready = false;
      while (!sim7680_check_sim(&sim_ready) ||
             !sim_ready) { // Kiểm tra SIM có sẵn sàng hay không
        printf(
            "[TaskMQTT] Cho nhan the SIM...\n"); // nếu luôn in ra dòng này thì
                                                 // có thể SIM chưa được gắn
                                                 // hoặc chưa nhận diện được
        vTaskDelay(pdMS_TO_TICKS(2000));
      }
      printf("[TaskMQTT] SIM san sang.\n");

      state = MQTT_STATE_CONNECT;
      break;
    }

    case MQTT_STATE_CONNECT: {
      printf("[TaskMQTT] Trang thai: CONNECT\n");
      if (!mqtt_init()) {
        printf("[TaskMQTT] Lỗi mqtt_init(). Thu lai sau 5s...\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
        break; // Thu lai state CONNECT
      }

      if (mqtt_connect()) {
        state = MQTT_STATE_SUBSCRIBE;
      } else {
        printf("[TaskMQTT] Lỗi mqtt_connect(). Thu lai sau 5s...\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
      }
      break;
    }

    case MQTT_STATE_SUBSCRIBE: {
      printf("[TaskMQTT] Trang thai: SUBSCRIBE\n");
      if (mqtt_subscribe(MQTT_TOPIC_COMMAND)) {
        printf("[TaskMQTT] Subscribe thanh cong: %s\n", MQTT_TOPIC_COMMAND);
        state = MQTT_STATE_RUNNING;
        last_pub_time = xTaskGetTickCount(); // Khoi tao thoi gian pub
      } else {
        printf("[TaskMQTT] Loi Subscribe, thu reconnect...\n");
        state = MQTT_STATE_RECONNECT;
      }
      break;
    }

    case MQTT_STATE_RUNNING: {
      // 1. LUÔN LUÔN xử lý tin nhắn đến trước
      mqtt_process();

      // 2. Publish theo chu kỳ
      TickType_t now = xTaskGetTickCount();
      if ((now - last_pub_time) * portTICK_PERIOD_MS >=
          MQTT_PUBLISH_PERIOD_MS) {
        last_pub_time = now;

        // Lay du lieu moi nhat
        BMI160_Physical_t bmi = {0};
        neo6m_data_t gps = {0};
        bool has_bmi = BMI160_GetLastData(&bmi);
        bool has_gps = GPS_GetLastData(&gps);

        char payload[256];
        snprintf(payload, sizeof(payload),
                 "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f,\"spd\":%.1f,"
                 "\"accX\":%.2f,\"accY\":%.2f,\"accZ\":%.2f}",
                 has_gps ? gps.latitude : 0.0f, has_gps ? gps.longitude : 0.0f,
                 has_gps ? gps.altitude_m : 0.0f,
                 has_gps ? gps.speed_kmh : 0.0f, has_bmi ? bmi.acc_x_g : 0.0f,
                 has_bmi ? bmi.acc_y_g : 0.0f, has_bmi ? bmi.acc_z_g : 0.0f);

        printf("[TaskMQTT] Publishing: %s\n", payload);
        if (!mqtt_publish(MQTT_TOPIC_TELEMETRY, payload, false)) {
          printf("[TaskMQTT] Publish that bai! Chuyen sang RECONNECT.\n");
          state = MQTT_STATE_RECONNECT;
        }
      }

      // Nhuong CPU cho task khac
      vTaskDelay(pdMS_TO_TICKS(100));
      break;
    }

    case MQTT_STATE_RECONNECT: {
      printf("[TaskMQTT] Trang thai: RECONNECT\n");
      mqtt_disconnect();
      vTaskDelay(pdMS_TO_TICKS(5000)); // Nghi 5s truoc khi thu ket noi lai
      state = MQTT_STATE_CONNECT;      // Khong can init SIM lai
      break;
    }
    }
  }
}

// ================   TASK: BUZZER                         ==============
// Độ ưu tiên thấp nhất: buzzer chỉ phát âm thanh, không ảnh hưởng telemetry.
#define PRIORITY_TASK_BUZZER (tskIDLE_PRIORITY + 1)
#define BUZZER_TASK_STACK_SIZE 512 // Chỉ cần GPIO + vTaskDelay, stack nhỏ là đủ
#define BUZZER_QUEUE_LEN 8         // Tối đa 8 lệnh chờ trong queue

static void TaskBuzzer(void *pvParameters) {
  (void)pvParameters;

  // B1: Khởi tạo GPIO còi
  Buzzer_Init();
  printf("[TaskBuzzer] Khoi tao buzzer GPIO %d thanh cong.\n", BUZZER_PIN);

  // B2: Phát 2 bíp báo boot OK -- dùng vTaskDelay thay sleep_ms
  gpio_put(BUZZER_PIN, 1);
  vTaskDelay(pdMS_TO_TICKS(BUZZER_BOOT_OK_BEEP_MS));
  gpio_put(BUZZER_PIN, 0);
  vTaskDelay(pdMS_TO_TICKS(BUZZER_BOOT_OK_GAP_MS));
  gpio_put(BUZZER_PIN, 1);
  vTaskDelay(pdMS_TO_TICKS(BUZZER_BOOT_OK_BEEP_MS));
  gpio_put(BUZZER_PIN, 0);
  printf("[TaskBuzzer] Boot OK beep phat xong.\n");

  // B3: Vòng lặp chính -- chờ lệnh từ queue, xử lý từng lệnh
  bool alarm_active = false;
  BuzzerCmd_t cmd;
  for (;;) {
    // Nếu đang alarm: chờ lệnh tối đa 3s rồi tự bíp; nếu không: chờ vô hạn
    TickType_t wait =
        alarm_active ? pdMS_TO_TICKS(BUZZER_ALARM_PERIOD_MS) : portMAX_DELAY;

    if (xQueueReceive(g_buzzer_queue, &cmd, wait) == pdTRUE) {
      switch (cmd) {
      case BUZZER_CMD_BOOT_OK:
        gpio_put(BUZZER_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(BUZZER_BOOT_OK_BEEP_MS));
        gpio_put(BUZZER_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(BUZZER_BOOT_OK_GAP_MS));
        gpio_put(BUZZER_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(BUZZER_BOOT_OK_BEEP_MS));
        gpio_put(BUZZER_PIN, 0);
        break;

      case BUZZER_CMD_BOOT_ERROR:
        gpio_put(BUZZER_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(BUZZER_ERROR_BEEP_MS));
        gpio_put(BUZZER_PIN, 0);
        break;

      case BUZZER_CMD_BEEP_SHORT:
        gpio_put(BUZZER_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_put(BUZZER_PIN, 0);
        break;

      case BUZZER_CMD_BEEP_LONG:
        gpio_put(BUZZER_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_put(BUZZER_PIN, 0);
        break;

      case BUZZER_CMD_ALARM:
        alarm_active = true;
        printf("[TaskBuzzer] Che do ALARM bat.\n");
        break;

      case BUZZER_CMD_ALARM_STOP:
        alarm_active = false;
        gpio_put(BUZZER_PIN, 0);
        printf("[TaskBuzzer] Che do ALARM tat.\n");
        break;

      default:
        break;
      }
    } else {
      // Timeout -> đang ở chế độ alarm, phát 1 bíp định vị
      if (alarm_active) {
        gpio_put(BUZZER_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(BUZZER_ALARM_BEEP_MS));
        gpio_put(BUZZER_PIN, 0);
      }
    }
  }
}

// ================   TASK: UI (TFT & XPT)   ==============
// Tham so (mutex SPI0) duoc truyen qua UI_TaskParams_t.
static UI_TaskParams_t g_ui_params;

// ================   HOOK BAT BUOC CUA FREERTOS            ========
extern "C" void vApplicationMallocFailedHook(void) {
  // Duoc goi khi pvPortMalloc() het heap (vd: khong du bo nho de tao
  // task/queue/mutex). In ra loi RO RANG thay vi de chuong trinh "treo im lang"
  // nhu truoc day, giup debug nhanh hon neu sau nay them task/bien moi lam vuot
  // configTOTAL_HEAP_SIZE.
  printf("!!! LOI: FreeRTOS het HEAP (pvPortMalloc that bai)! Hay tang "
         "configTOTAL_HEAP_SIZE. !!!\n");
  taskDISABLE_INTERRUPTS();
  for (;;) {
    tight_loop_contents();
  }
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                              char *pcTaskName) {
  (void)xTask;
  printf("!!! STACK OVERFLOW o task: %s !!!\n", pcTaskName);
  taskDISABLE_INTERRUPTS();
  for (;;) {
    tight_loop_contents();
  }
}

// ================                MAIN                      =============
int main() {
  stdio_init_all();
  sleep_ms(5000);

  // Khoi tao tai nguyen dung chung TRUOC khi tao task su dung no
  g_mutex_gps_data = xSemaphoreCreateMutex();
  g_mutex_bmi_data = xSemaphoreCreateMutex();
  g_mutex_spi0 = xSemaphoreCreateMutex();
  g_buzzer_queue = xQueueCreate(BUZZER_QUEUE_LEN, sizeof(BuzzerCmd_t));
  if (g_mutex_gps_data == NULL || g_mutex_bmi_data == NULL ||
      g_mutex_spi0 == NULL || g_buzzer_queue == NULL) {
    printf("[main] Khong the tao mutex/queue!\n");
    while (true)
      tight_loop_contents();
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
  BaseType_t ok_SDCARD =
      xTaskCreate(TaskSDCard, "SDCARD", SDCARD_TASK_STACK_SIZE, NULL,
                  PRIORITY_TASK_SDCARD, NULL);
  BaseType_t ok_MQTT = xTaskCreate(TaskMQTT, "MQTT", MQTT_TASK_STACK_SIZE, NULL,
                                   PRIORITY_TASK_MQTT, NULL);
  BaseType_t ok_BUZZER =
      xTaskCreate(TaskBuzzer, "BUZZER", BUZZER_TASK_STACK_SIZE, NULL,
                  PRIORITY_TASK_BUZZER, NULL);
  // cap nhat tham so UI truoc khi tao task
  g_ui_params.mutex_spi0 = g_mutex_spi0;

  BaseType_t ok_UI = xTaskCreate(TaskUI, "UI", 2048, &g_ui_params,
                                 (tskIDLE_PRIORITY + 1), NULL);

  if (ok_BMI160 != pdPASS || ok_GPS != pdPASS || ok_SDCARD != pdPASS ||
      ok_UI != pdPASS || ok_BUZZER != pdPASS || ok_MQTT != pdPASS) {
    printf("Loi: khong the tao task!\n");
    while (
        true) { // nếu không thể tạo task, vòng lặp vô hạn để dừng chương trình
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
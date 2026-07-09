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

// ================Định nghĩa mức độ ưu tiên (Priority) cho các Task trong FreeRTOS==============
#define PRIORITY_TASK_BMI160 (tskIDLE_PRIORITY + 3)
#define PRIORITY_TASK_GPS (tskIDLE_PRIORITY + 2)

// ================     Tài nguyên dùng chung (mutex...)   ==============
static SemaphoreHandle_t g_mutex_gps_data = NULL; // mutex bảo vệ dữ liệu GPS (g_gps_data) khi được truy cập bởi nhiều task
static SemaphoreHandle_t g_mutex_bmi_data = NULL; // mutex bảo vệ dữ liệu BMI160 (g_bmi_data) khi được truy cập bởi nhiều task

// ================          TASK: BMI160 (IMU)            ==============
#define BMI160_TASK_STACK_SIZE 1024     // Cấp phát kích thước vùng nhớ Stack cho task
#define BMI160_READ_PERIOD_MS 200       // Chu kỳ chạy của task. Cứ mỗi 200ms, task sẽ thức dậy một lần để đọc bộ nhớ đệm FIFO của cảm biến.
#define BMI160_FIFO_SCRATCH_SIZE 256    // Kích thước mảng đệm tạm thời (bằng byte) dùng để chứa dữ liệu thô đọc ra từ thanh ghi FIFO
#define BMI160_FIFO_MAX_FRAMES 32       // số frame tối đa có thể đọc trong một lần đọc FIFO (tăng nếu muốn đọc nhiều frame hơn)
#define BMI160_MUTEX_WAIT_MS 50         // Thời gian tối đa (50ms) mà một hàm chấp nhận "đứng đợi" để lấy được Mutex

static BMI160_Physical_t g_bmi_data;
static bool g_bmi_data_valid = false; // true kể từ khi có dữ liệu BMI160 hợp lệ đầu tiên, false nếu chưa có dữ liệu hợp lệ
bool BMI160_GetLastData(BMI160_Physical_t *out) {
  if (out == NULL || g_mutex_bmi_data == NULL) return false;// nếu con trỏ đầu ra là NULL hoặc mutex chưa được khởi tạo, trả về false

  bool ok = false; 
  if (xSemaphoreTake(g_mutex_bmi_data, pdMS_TO_TICKS(BMI160_MUTEX_WAIT_MS)) == pdTRUE) { // nếu lấy được mutex thành công trong thời gian chờ, thực hiện truy cập dữ liệu BMI160
    if (g_bmi_data_valid) { // nếu dữ liệu BMI160 hợp lệ, sao chép dữ liệu vào con trỏ đầu ra và đặt ok = true
      *out = g_bmi_data;
      ok = true;
    }
    xSemaphoreGive(g_mutex_bmi_data);// giải phóng mutex sau khi truy cập xong
  }
  return ok;
}
static void TaskBMI160(void *pvParameters) {
  (void)pvParameters; // 

  bmi_dev_t bmi; // khai báo biến bmi với kiểu dữ liệu bmi_dev_t
  BMI160_Config_t config = { 
    .accel_range = BMI_ACC_RANGE_4G,
    .gyro_range = BMI_GYR_RANGE_500DPS,
    .accel_odr = BMI_ACC_CONFIG_DEFAULT, // 100Hz
    .gyro_odr = BMI_GYR_CONFIG_DEFAULT,  // 100Hz
  };
  while (BMI160_Init(&bmi, BMI160_I2C_PORT, BMI160_I2C_ADDR, &config) != BMI_OK) {
    printf("[TaskBMI160] Khoi tao BMI160 that bai, thu lai...\n");
    vTaskDelay(pdMS_TO_TICKS(500));// nếu init thất bại, lặp lại sau 500ms
  }
  // cấu hình FIFO: bat acc + gyro, header mode de de giai ma
  BMI160_FIFO_Config_t fifo_cfg = {
      .acc_en = true,
      .gyr_en = true,
      .header_en = true,
      .time_en = false,
      .watermark = 0,
      .fifo_mode = BMI160_FIFO_MODE_OVERWRITE,
      .overrun_en = true,
  };
  BMI160_FIFO_Config(&bmi, &fifo_cfg);// cấu hình FIFO
  uint8_t scratch_buf[BMI160_FIFO_SCRATCH_SIZE]; // khởi tạo bộ nhớ đệm tạm thời
  BMI160_FIFO_Frame_t frames[BMI160_FIFO_MAX_FRAMES]; // khởi tạo bộ nhớ đệm để lưu trữ các frame
  BMI160_FIFO_Result_t result; // khởi tạo cấu trúc để lưu trữ kết quả
  TickType_t lastWake = xTaskGetTickCount(); // lấy thời gian hiện tại
  for (;;) { // vòng lặp vô hạn để đọc dữ liệu BMI160
    // đọc dữ liệu FIFO và phân tích dữ liệu thành các frame, lưu trữ kết quả vào biến result. Nếu có lỗi trong quá trình đọc FIFO, in ra thông báo lỗi.
    BMI_Status status = BMI160_FIFO_ReadAndParse(&bmi, scratch_buf, BMI160_FIFO_SCRATCH_SIZE, frames, BMI160_FIFO_MAX_FRAMES, &result);


    // Nếu đọc và phân tích dữ liệu FIFO thành công và có ít nhất một frame dữ liệu, 
    // thực hiện chuyển đổi frame cuối cùng sang đơn vị vật lý (g, dps)
    // và cập nhật dữ liệu BMI160 mới nhất cho các task khác thông qua mutex. 
    // Nếu có lỗi trong quá trình đọc FIFO hoặc chuyển đổi đơn vị, in ra thông báo lỗi.
    if (status == BMI_OK && result.frame_count > 0) {
      BMI160_FIFO_Frame_t *last = &frames[result.frame_count - 1]; // lấy frame cuối cùng trong mảng frames
      BMI160_Physical_t phys; // khai báo biến phys để lưu trữ dữ liệu vật lý sau khi chuyển đổi
      if (BMI160_Algo_ConvertFrame(last, bmi.config.accel_range, bmi.config.gyro_range, &phys) == BMI_OK) {
        if (xSemaphoreTake(g_mutex_bmi_data, pdMS_TO_TICKS(BMI160_MUTEX_WAIT_MS)) == pdTRUE) {
            g_bmi_data = phys;
            g_bmi_data_valid = true;
            xSemaphoreGive(g_mutex_bmi_data);
        }

        TickType_t now = xTaskGetTickCount();
        // printf("[TaskBMI160] thoi gian hien tai = %u ms, ", (unsigned int)(now * portTICK_PERIOD_MS));
        // printf("[TaskBMI160] Thoi gian doc FIFO: %u ms\n", (unsigned int)(now - lastWake) * portTICK_PERIOD_MS);
        
        printf(" [TaskBMI160] frames=%u Acc(%.3f,%.3f,%.3f)g |%.3f|g Gyro(%.2f,%.2f,%.2f)dps\n",
              result.frame_count, 
              phys.acc_x_g, phys.acc_y_g, phys.acc_z_g, phys.acc_magnitude_g, 
              phys.gyr_x_dps, phys.gyr_y_dps, phys.gyr_z_dps);
    }
    } else if (status != BMI_OK) {
      printf("[TaskBMI160] Loi doc FIFO!\n");
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(BMI160_READ_PERIOD_MS)); // delay cho đến khi chu kỳ tiếp theo bắt đầu
    // lastWake : là thời điểm mà task được đánh thức lần cuối cùng, được cập nhật bởi hàm vTaskDelayUntil() để đảm bảo rằng task sẽ được đánh thức đúng chu kỳ định sẵn.
  }
}

// ================          TASK: GPS                    ==============
#define GPS_TASK_STACK_SIZE 1024 // word (4 byte/word tren RP2040)
#define GPS_MUTEX_WAIT_MS 50 // thoi gian toi da cho mutex khi cap nhat/doc du lieu

// ----------------------------------------------------------------
// BẬT/TẮT chế độ giả lập GPS (không cần phần cứng, không cần tín hiệu vệ tinh)
// Khi ở trong nhà hoặc muốn test: giữ nguyên dòng dưới
// Khi dùng phần cứng thật ngoài trời:  comment dòng dưới lại
// ----------------------------------------------------------------
#define GPS_SIMULATE

#ifdef GPS_SIMULATE
// Tọa độ gốc giả lập (Hà Nội - Hoàn Kiếm)
#define GPS_SIM_LAT_BASE    21.028511f
#define GPS_SIM_LON_BASE   105.834160f
#define GPS_SIM_ALT_M       15.0f
#define GPS_SIM_SPEED_KMH   12.5f
#define GPS_SIM_SATELLITES  8
// Mỗi chu kỳ vị trí dịch chuyển một lượng nhỏ để giả lập chuyển động
#define GPS_SIM_LAT_STEP    0.000050f  // ~5.5 m mỗi giây về hướng Bắc
#define GPS_SIM_LON_STEP    0.000045f  // ~4.5 m mỗi giây về hướng Đông
#define GPS_SIM_PERIOD_MS   1000       // Gửi dữ liệu mỗi 1 giây (đúng như module thật)
#endif // GPS_SIMULATE

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
    printf("[TaskGPS] Che do GIA LAP GPS (GPS_SIMULATE bat). Khong can phan cung.\n");

    static neo6m_data_t sim = {
        .latitude    = GPS_SIM_LAT_BASE,
        .longitude   = GPS_SIM_LON_BASE,
        .altitude_m  = GPS_SIM_ALT_M,
        .fix_quality = NEO6M_FIX_GPS,
        .satellites  = GPS_SIM_SATELLITES,
        .hdop        = 1.2f,
        .speed_knots = GPS_SIM_SPEED_KMH / 1.852f,
        .speed_kmh   = GPS_SIM_SPEED_KMH,
        .course_deg  = 45.0f,
        .hour        = 7,
        .minute      = 0,
        .second      = 0,
        .day         = 9,
        .month       = 7,
        .year        = 2025,
        .is_valid    = true,
    };

    TickType_t lastWake = xTaskGetTickCount();
    for (;;) {
        // Cập nhật thời gian giả (tăng 1 giây mỗi chu kỳ)
        sim.second++;
        if (sim.second >= 60) { sim.second = 0; sim.minute++; }
        if (sim.minute >= 60) { sim.minute = 0; sim.hour++;   }
        if (sim.hour   >= 24) { sim.hour   = 0; }

        // Dịch chuyển tọa độ nhỏ để giả lập chuyển động
        sim.latitude  += GPS_SIM_LAT_STEP;
        sim.longitude += GPS_SIM_LON_STEP;

        // Ghi vào biến toàn cục (bảo vệ bằng mutex)
        if (xSemaphoreTake(g_mutex_gps_data, pdMS_TO_TICKS(GPS_MUTEX_WAIT_MS)) == pdTRUE) {
            g_gps_data = sim;
            xSemaphoreGive(g_mutex_gps_data);
        }

        TickType_t now = xTaskGetTickCount();
        printf("[TaskGPS][SIM] t=%ums Fix OK - vido=%.6f kinhdo=%.6f docao=%.1fm sat=%u tocdo=%.1fkm/h %02u:%02u:%02u\n",
               (unsigned int)(now * portTICK_PERIOD_MS),
               sim.latitude, sim.longitude, sim.altitude_m,
               (unsigned int)sim.satellites, sim.speed_kmh,
               (unsigned int)sim.hour, (unsigned int)sim.minute, (unsigned int)sim.second);

        // Delay chính xác đến chu kỳ tiếp theo (1 giây)
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(GPS_SIM_PERIOD_MS));
    }

#else
    // ---- Chế độ thực: đọc dữ liệu từ module NEO-6M qua UART ----
    neo6m_dev_t gps_dev;

    while (NEO6M_Init(&gps_dev, NEO6M_UART_PORT, NEO6M_PIN_TX, NEO6M_PIN_RX, NEO6M_BAUDRATE) != NEO6M_OK) {
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
                if (NEO6M_ParseGGA(gps_dev.nmea_buf, &tmp) == NEO6M_OK) parsed = true;
            } else if (strncmp(gps_dev.nmea_buf, NEO6M_NMEA_RMC, 6) == 0) {
                if (NEO6M_ParseRMC(gps_dev.nmea_buf, &tmp) == NEO6M_OK) parsed = true;
            }

            TickType_t t_parse_done = xTaskGetTickCount();
            uint32_t parse_ms = (t_parse_done - t_read_done) * portTICK_PERIOD_MS;

            printf("[TaskGPS] doc=%ums parse=%ums [%s] -> %s\n",
                   (unsigned int)read_ms,
                   (unsigned int)parse_ms,
                   gps_dev.nmea_buf,
                   parsed ? "OK" : "BO QUA");

            if (parsed) {
                if (xSemaphoreTake(g_mutex_gps_data, pdMS_TO_TICKS(GPS_MUTEX_WAIT_MS)) == pdTRUE) {
                    g_gps_data = tmp;
                    xSemaphoreGive(g_mutex_gps_data);
                }

                if (strncmp(gps_dev.nmea_buf, NEO6M_NMEA_RMC, 6) == 0) {
                    TickType_t now_gps = xTaskGetTickCount();
                    if (tmp.is_valid) {
                        printf("[TaskGPS] t=%ums Fix OK - vido=%.6f kinhdo=%.6f docao=%.1fm sovetinh=%u tocdo=%.1fkm/h\n",
                               (unsigned int)(now_gps * portTICK_PERIOD_MS),
                               tmp.latitude, tmp.longitude, tmp.altitude_m, tmp.satellites, tmp.speed_kmh);
                    } else {
                        printf("[TaskGPS] t=%ums Chua co fix (sat=%u)\n",
                               (unsigned int)(now_gps * portTICK_PERIOD_MS), tmp.satellites);
                    }
                }
            }
        }
    }
#endif // GPS_SIMULATE
}
// ================   TASK: MQTT            ========
// nhiệm vụ : 
// 1. kết nối mạng (nếu chưa kết nối)
// 2. kết nối MQTT (nếu chưa kết nối)
// 3. lấy dữ liệu GPS và BMI160 từ các biến toàn cục được bảo vệ bởi mutex
// 4. gửi dữ liệu GPS và BMI160 lên server MQTT



// ================   HOOK BAT BUOC CUA FREERTOS            ========
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
  sleep_ms(5000); // cho mo cong USB-serial de khong mat log dau tien (bo neu
                  // dung UART thuong)

  // Khoi tao tai nguyen dung chung TRUOC khi tao task su dung no
  g_mutex_gps_data = xSemaphoreCreateMutex();
  g_mutex_bmi_data = xSemaphoreCreateMutex();
  if (g_mutex_gps_data == NULL || g_mutex_bmi_data == NULL) {
    printf("[main] Khong the tao mutex!\n");
    while (true) tight_loop_contents();
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
  BaseType_t ok_GPS = xTaskCreate(TaskGPS, "GPS", GPS_TASK_STACK_SIZE, NULL,  PRIORITY_TASK_GPS, NULL);

  if (ok_BMI160 != pdPASS) {
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
#ifndef BUZZER_H
#define BUZZER_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// -------------------- cấu hình chân GPIO --------------------
// Giả định còi là loại active buzzer (chỉ cần cấp mức HIGH/LOW là kêu/tắt),
// điều khiển trực tiếp hoặc qua transistor/MOSFET đóng vai trò công tắc.
#define BUZZER_PIN              3     // Còi báo -> GPIO 2

// -------------------- thời gian mặc định (ms) --------------------
#define BUZZER_BOOT_OK_BEEP_MS   120    // Độ dài mỗi tiếng bíp khi boot OK
#define BUZZER_BOOT_OK_GAP_MS    120    // Khoảng nghỉ giữa 2 tiếng bíp boot OK
#define BUZZER_ERROR_BEEP_MS     1500   // Độ dài tiếng "biiiíp" báo lỗi nghiêm trọng
#define BUZZER_ALARM_BEEP_MS     200    // Độ dài 1 tiếng bíp ở chế độ định vị (locate)
#define BUZZER_ALARM_PERIOD_MS   3000   // Chu kỳ lặp lại của chế độ định vị (3 giây/lần)

// -------------------- API điều khiển còi --------------------

// Khởi tạo chân GPIO điều khiển còi (gọi 1 lần lúc đầu chương trình)
void Buzzer_Init(void);

// Bật còi (kêu liên tục cho tới khi gọi Buzzer_Off)
void Buzzer_On(void);

// Tắt còi
void Buzzer_Off(void);

// Kêu 1 tiếng bíp đơn với thời gian tùy chọn -- HÀM CHẶN LUỒNG (blocking)
void Buzzer_Beep(uint32_t duration_ms);

// Kêu "count" tiếng bíp liên tiếp, mỗi tiếng dài on_ms, cách nhau gap_ms -- blocking
void Buzzer_BeepPattern(uint8_t count, uint32_t on_ms, uint32_t gap_ms);

// Báo hiệu khởi động thành công: 2 tiếng "bíp bíp" ngắn
void Buzzer_BootOK(void);

// Báo hiệu lỗi nghiêm trọng lúc khởi động: 1 tiếng "biiiíp" dài liên tục
void Buzzer_BootError(void);

// Chế độ phát tín hiệu định vị (locate mode) -- NON-BLOCKING, gọi liên tục
// trong vòng lặp chính; hàm tự kêu 1 tiếng lớn mỗi khi đến chu kỳ
// BUZZER_ALARM_PERIOD_MS mà không cần sleep_ms() chặn luồng bên ngoài.
void Buzzer_AlarmTick(void);

#ifdef __cplusplus
}
#endif

#endif // BUZZER_H
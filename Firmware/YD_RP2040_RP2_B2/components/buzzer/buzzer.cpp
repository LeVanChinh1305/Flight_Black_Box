#include "buzzer.h"

// biến nội bộ lưu mốc thời gian cho chế độ định vị (non-blocking)
static absolute_time_t s_next_alarm_time;
static bool s_alarm_initialized = false;

void Buzzer_Init(void) {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0); // tắt còi mặc định
}

void Buzzer_On(void) {
    gpio_put(BUZZER_PIN, 1);
}

void Buzzer_Off(void) {
    gpio_put(BUZZER_PIN, 0);
}

void Buzzer_Beep(uint32_t duration_ms) {
    Buzzer_On();
    sleep_ms(duration_ms);
    Buzzer_Off();
}

void Buzzer_BeepPattern(uint8_t count, uint32_t on_ms, uint32_t gap_ms) {
    for (uint8_t i = 0; i < count; i++) {
        Buzzer_Beep(on_ms);
        // không nghỉ sau tiếng bíp cuối cùng
        if (i < count - 1) {
            sleep_ms(gap_ms);
        }
    }
}

void Buzzer_BootOK(void) {
    // 2 tiếng "bíp bíp" ngắn: toàn bộ hệ thống khởi động hoàn hảo
    Buzzer_BeepPattern(2, BUZZER_BOOT_OK_BEEP_MS, BUZZER_BOOT_OK_GAP_MS);
}

void Buzzer_BootError(void) {
    // 1 tiếng "biiiíp" dài liên tục: hệ thống gặp lỗi nghiêm trọng lúc khởi động
    Buzzer_Beep(BUZZER_ERROR_BEEP_MS);
}

void Buzzer_AlarmTick(void) {
    if (!s_alarm_initialized) {
        s_next_alarm_time = make_timeout_time_ms(BUZZER_ALARM_PERIOD_MS);
        s_alarm_initialized = true;
    }

    // Đến chu kỳ định vị chưa? (so sánh mốc thời gian tuyệt đối, không dùng sleep_ms tích lũy)
    if (absolute_time_diff_us(get_absolute_time(), s_next_alarm_time) <= 0) {
        Buzzer_Beep(BUZZER_ALARM_BEEP_MS); // tiếng bíp ngắn, chấp nhận blocking vì thời gian rất nhỏ
        s_next_alarm_time = make_timeout_time_ms(BUZZER_ALARM_PERIOD_MS);
    }
}
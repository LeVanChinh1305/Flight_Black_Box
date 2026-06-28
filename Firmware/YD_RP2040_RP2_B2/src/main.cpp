#include <stdio.h>
#include "pico/stdlib.h"
#include "components/sim7080g/sim7080g.h"

int main()
{
    stdio_init_all();
    sleep_ms(2000);

    printf("\n--- HE THONG HOP DEN: QUET THONG SO SIM7080G ---\n");

    if (sim_init() != SIM_OK) {
        printf("[FATAL] Khoi tao Module SIM that bai!\n");
        while (1) { tight_loop_contents(); }
    }

    if (sim_check_sim() != SIM_OK) {
        printf("[FATAL] The SIM chua lap hoac bi loi!\n");
        while (1) { tight_loop_contents(); }
    }

    char dummy[64];

    // ── Cấu hình CAT-M cho SIM Viettel 4G ──
    sim_send_at("AT+CNMP=38\r\n",  dummy, sizeof(dummy), SIM_DEFAULT_TIMEOUT_MS); // LTE only
    sim_send_at("AT+CMNB=1\r\n",   dummy, sizeof(dummy), SIM_DEFAULT_TIMEOUT_MS); // CAT-M only
    sim_send_at("AT+CBANDCFG=\"CAT-M\",3,28\r\n", dummy, sizeof(dummy), SIM_DEFAULT_TIMEOUT_MS); // Band 3 (1800MHz) + Band 28 (700MHz) — Viettel VN
    sim_send_at("AT+CEREG=2\r\n",  dummy, sizeof(dummy), SIM_DEFAULT_TIMEOUT_MS); // Bật URC đăng ký mạng

    // Reset radio để áp dụng cấu hình mới
    printf("[SIM] Reset radio de ap dung cau hinh moi...\n");
    sim_send_at("AT+CFUN=0\r\n", dummy, sizeof(dummy), 3000);
    sleep_ms(1000);
    sim_send_at("AT+CFUN=1\r\n", dummy, sizeof(dummy), 3000);
    sleep_ms(8000); // Chờ radio ổn định

    // Thu thập diagnostics
    sim_get_all_diagnostics();

    // Chờ đăng ký mạng CAT-M tối đa 60 giây
    printf("[SIM] Cho dang ky mang CAT-M Viettel...\n");
    if (sim_wait_network(60000) != SIM_OK) {
        printf("[CANH BAO] Khong dang ky duoc mang sau 60 giay.\n");
        printf("[CANH BAO] Kiem tra: antenna, vung phu song CAT-M Viettel khu vuc ban.\n");
    }

    // Vòng lặp giám sát liên tục 5 giây/lần
    printf("\nChuyen sang che do tracking lien tuc...\n");
    while (1) {
        printf("\n--- TRACKING STATUS ---\n");
        sim_send_at("AT+CSQ\r\n",    NULL, 0, SIM_DEFAULT_TIMEOUT_MS); // Tín hiệu
        sim_send_at("AT+CEREG?\r\n", NULL, 0, SIM_DEFAULT_TIMEOUT_MS); // Trạng thái mạng
        sim_send_at("AT+COPS?\r\n",  NULL, 0, SIM_DEFAULT_TIMEOUT_MS); // Nhà mạng
        sleep_ms(5000);
    }
}
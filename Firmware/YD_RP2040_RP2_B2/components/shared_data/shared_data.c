#include "shared_data.h"

void record_error(uint8_t error_code) {
    if (g_mutex_error_list) {
        xSemaphoreTake(g_mutex_error_list, portMAX_DELAY);
    }
    if (g_error_list.error_count < MAX_ERRORS) {
        g_error_list.error_codes[g_error_list.error_count] = error_code;
        g_error_list.error_count++;
    }
    if (g_mutex_error_list) {
        xSemaphoreGive(g_mutex_error_list);
    }
}

#include "Data_Sender.h"
#include <string.h>
#include <stdio.h>

// ===========================================================================
// HÀM NỘI BỘ
// ===========================================================================

/**
 * @brief  Tính checksum XOR của phần payload trong frame.
 *         Payload = tất cả byte từ sau header đến trước checksum.
 * @param  frame  Con trỏ tới UartFrame_t đã điền đủ dữ liệu (trừ checksum).
 * @retval Giá trị checksum XOR.
 */
static uint8_t calc_checksum(const UartFrame_t *frame) {
    const uint8_t *p         = (const uint8_t *)frame;
    uint8_t        cs        = 0;
    // Bắt đầu từ byte thứ 2 (bỏ header[0..1])
    // Kết thúc trước checksum và tail (bỏ 2 byte cuối)
    uint16_t payload_start   = sizeof(frame->header);
    uint16_t payload_end     = sizeof(UartFrame_t) - sizeof(frame->checksum) - sizeof(frame->tail);

    for (uint16_t i = payload_start; i < payload_end; i++) {
        cs ^= p[i];
    }
    return cs;
}

/**
 * @brief  Điền ProcessedData_t vào UartFrame_t và tính checksum.
 */
static void build_frame(const ProcessedData_t *data, UartFrame_t *frame) {
    frame->header[0]   = DATA_SENDER_FRAME_HEADER_0;
    frame->header[1]   = DATA_SENDER_FRAME_HEADER_1;
    frame->timestamp_ms = data->timestamp_ms;
    frame->acc_x_g     = data->acc_x_g;
    frame->acc_y_g     = data->acc_y_g;
    frame->acc_z_g     = data->acc_z_g;
    frame->gyr_x_dps   = data->gyr_x_dps;
    frame->gyr_y_dps   = data->gyr_y_dps;
    frame->gyr_z_dps   = data->gyr_z_dps;
    frame->temp_c      = data->temp_c;
    frame->pitch_deg   = data->pitch_deg;
    frame->roll_deg    = data->roll_deg;
    frame->checksum    = calc_checksum(frame);
    frame->tail        = DATA_SENDER_FRAME_TAIL;
}

// ===========================================================================
// API CÔNG KHAI
// ===========================================================================

int Data_Sender_Init(void) {
    // UART2 đã được init trong main trước khi gọi hàm này.
    // Để trống — chỗ này có thể thêm handshake với ESP32 nếu cần sau này.
    return 0;
}

void Data_Sender_Task(void *pvParameters) {
    (void)pvParameters;

    ProcessedData_t proc;
    UartFrame_t     frame;
    HAL_StatusTypeDef status;

    for (;;) {
        // Block vô thời hạn, chờ Task_ProcessBMI gửi dữ liệu đã tính toán
        if (xQueueReceive(xQueueProcessed, &proc, portMAX_DELAY) == pdTRUE) {

            build_frame(&proc, &frame);

            status = HAL_UART_Transmit(&huart2,
                                       (uint8_t *)&frame,
                                       sizeof(UartFrame_t),
                                       DATA_SENDER_UART_TIMEOUT_MS);

            if (status != HAL_OK) {
                printf("[SEND] UART transmit error: %d\r\n", (int)status);
            }
        }
    }
}
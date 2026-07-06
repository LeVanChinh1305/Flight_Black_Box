#include "algorithm_bmi160.h"
#include <math.h>

// Bang do nhay Accelerometer theo Datasheet BMI160 muc 2.6 (LSB/g)
float BMI160_Algo_GetAccelSensitivity(uint8_t accel_range) {
    switch (accel_range) {
        case BMI_ACC_RANGE_2G:  return 16384.0f;
        case BMI_ACC_RANGE_4G:  return 8192.0f;
        case BMI_ACC_RANGE_8G:  return 4096.0f;
        case BMI_ACC_RANGE_16G: return 2048.0f;
        default: return 0.0f; // ma range khong hop le
    }
}

// Bang do nhay Gyroscope theo Datasheet BMI160 muc 2.7 (LSB / (do/giay))
float BMI160_Algo_GetGyroSensitivity(uint8_t gyro_range) {
    switch (gyro_range) {
        case BMI_GYR_RANGE_2000DPS: return 16.4f;
        case BMI_GYR_RANGE_1000DPS: return 32.8f;
        case BMI_GYR_RANGE_500DPS:  return 65.6f;
        case BMI_GYR_RANGE_250DPS:  return 131.2f;
        case BMI_GYR_RANGE_125DPS:  return 262.4f;
        default: return 0.0f; // ma range khong hop le
    }
}

BMI_Status BMI160_Algo_ConvertFrame(const BMI160_FIFO_Frame_t *frame,
                                     uint8_t accel_range, uint8_t gyro_range,
                                     BMI160_Physical_t *out) {
    if (frame == NULL || out == NULL) return BMI_ERROR;

    // Dien 0 truoc de tranh tra ve gia tri rac neu loi giua chung
    *out = (BMI160_Physical_t){0};

    float acc_sens = BMI160_Algo_GetAccelSensitivity(accel_range);
    float gyr_sens = BMI160_Algo_GetGyroSensitivity(gyro_range);
    if (acc_sens <= 0.0f || gyr_sens <= 0.0f) return BMI_ERROR; // range khong hop le

    if (frame->has_acc) {
        out->acc_x_g = (float)frame->acc_x / acc_sens;
        out->acc_y_g = (float)frame->acc_y / acc_sens;
        out->acc_z_g = (float)frame->acc_z / acc_sens;
        out->acc_magnitude_g = sqrtf(out->acc_x_g * out->acc_x_g +
                                      out->acc_y_g * out->acc_y_g +
                                      out->acc_z_g * out->acc_z_g);
    }

    if (frame->has_gyr) {
        out->gyr_x_dps = (float)frame->gyr_x / gyr_sens;
        out->gyr_y_dps = (float)frame->gyr_y / gyr_sens;
        out->gyr_z_dps = (float)frame->gyr_z / gyr_sens;
    }

    return BMI_OK;
}
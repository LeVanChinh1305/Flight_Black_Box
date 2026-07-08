#ifndef ALGORITHM_BMI160_H
#define ALGORITHM_BMI160_H

// Module thuat toan: quy doi du lieu tho (raw LSB) tu BMI160_FIFO_Frame_t
// sang don vi vat ly (g cho accel, do/giay cho gyro), khong dung nao khac
// ngoai chinh no + bmi160.h. Tach rieng khoi driver bmi160.cpp de driver chi
// lam nhiem vu doc/ghi thanh ghi, con "y nghia" cua so lieu nam o day.

#include "bmi160.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Ket qua da quy doi sang don vi vat ly cua 1 frame IMU
typedef struct {
    float acc_x_g;      // gia toc truc X, don vi: g (9.80665 m/s^2)
    float acc_y_g;      // gia toc truc Y, don vi: g
    float acc_z_g;      // gia toc truc Z, don vi: g
    float acc_magnitude_g; // do lon vector gia toc tong hop = sqrt(x^2+y^2+z^2), don vi: g
                            // (~1g khi dung yen, >>1g khi co va cham/rung manh - huu ich
                            //  cho blackbox de phat hien su kien bat thuong)

    float gyr_x_dps;    // van toc goc truc X, don vi: do/giay (deg/s)
    float gyr_y_dps;    // van toc goc truc Y, don vi: do/giay
    float gyr_z_dps;    // van toc goc truc Z, don vi: do/giay
} BMI160_Physical_t;

// Tra ve do nhay (sensitivity) LSB/g ung voi ma range accel (BMI_ACC_RANGE_xG).
// Tra ve 0.0f neu ma range khong hop le.
float BMI160_Algo_GetAccelSensitivity(uint8_t accel_range);

// Tra ve do nhay (sensitivity) LSB/(do/giay) ung voi ma range gyro (BMI_GYR_RANGE_xDPS).
// Tra ve 0.0f neu ma range khong hop le.
float BMI160_Algo_GetGyroSensitivity(uint8_t gyro_range);

// Quy doi 1 frame FIFO tho (LSB) sang don vi vat ly, dua theo range accel/gyro
// dang cau hinh cho cam bien (lay tu dev->config.accel_range / gyro_range).
// Tra ve BMI_ERROR neu tham so NULL hoac ma range khong hop le (out se duoc dien 0 trong truong hop loi, khong de gia tri rac).
BMI_Status BMI160_Algo_ConvertFrame(const BMI160_FIFO_Frame_t *frame, uint8_t accel_range, uint8_t gyro_range, BMI160_Physical_t *out);

#ifdef __cplusplus
}
#endif

#endif // ALGORITHM_BMI160_H
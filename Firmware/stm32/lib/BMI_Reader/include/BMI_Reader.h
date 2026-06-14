#ifndef BMI_READER_H
#define BMI_READER_H


#include "app_types.h"
#include "BMI160.h"

// CẤU HÌNH
#define BMI_READER_PERIOD_MS     10U     // 100 Hz
#define BMI_READER_ERR_THRESHOLD 5U

// API

int BMI_Reader_Init(bmi_dev_t *dev);

void BMI_Reader_Task(void *pvParameters);

#endif /* BMI_READER_H */
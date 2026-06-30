#ifndef FONT5X7_H
#define FONT5X7_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// --------------------bảng font bitmap 5x7-------------------
//
// Font cơ bản dùng cho hiển thị chữ trên màn hình TFT.
// Mỗi ký tự được mã hoá bằng 5 byte (5 cột), mỗi byte là 1 cột pixel
// theo chiều dọc 8 bit (bit thấp nằm phía trên).
//
// Bảng hỗ trợ đầy đủ ký tự ASCII in được, từ 32 (space) đến 126 (~).
// index truy cập = (mã_ascii - 32)

extern const uint8_t font5x7[][5];

#ifdef __cplusplus
}
#endif

#endif // FONT5X7_H
# Flight Black Box Firmware

## Tổng quan dự án (Project Overview)
Dự án Firmware cho hệ thống "Hộp đen" (Flight Black Box), sử dụng vi điều khiển trung tâm **RP2040**. Hệ thống có nhiệm vụ thu thập, tính toán xử lý dữ liệu từ các cảm biến, lưu trữ cục bộ và truyền dữ liệu thời gian thực lên Webserver qua giao thức MQTT. Ngoài ra, hệ thống hỗ trợ tính năng truy xuất dữ liệu từ xa theo yêu cầu từ Webserver.

## Tính năng chính (Key Features)
- **Thu thập dữ liệu:** Đọc dữ liệu liên tục từ module cảm biến IMU (gia tốc, góc quay) **BMI160** và module định vị **GPS6N**.
- **Xử lý số liệu:** Tính toán, phân tích các thông số chuyển động, tọa độ và đóng gói dữ liệu.
- **Lưu trữ cục bộ:** Ghi nhật ký thông tin, dữ liệu bay an toàn vào **thẻ nhớ (SD Card)** để đảm bảo không mất mát dữ liệu.
- **Kết nối IoT (Thời gian thực):** Sử dụng **Module SIM** để kết nối internet và gửi tín hiệu telemetry lên Webserver thông qua giao thức **MQTT**.
- **Truy xuất dữ liệu từ xa:** Webserver có thể gửi yêu cầu (request) xuống RP2040. Khi nhận được lệnh, thiết bị sẽ đọc lại các thông tin đã lưu trong thẻ nhớ và gửi ngược lên server.

## Phần cứng sử dụng (Hardware Components)
- **Vi điều khiển:** RP2040 (Raspberry Pi RP2040)
- **Cảm biến IMU:** Module BMI160
- **Cảm biến vị trí:** Module GPS6N
- **Bộ nhớ lưu trữ:** Module SD Card
- **Giao tiếp mạng:** Module SIM (hỗ trợ kết nối GPRS/3G/4G, giao tiếp AT Command hoặc thư viện tích hợp)

## Luồng hoạt động cơ bản (Data Flow)
1. **Đọc Cảm biến:** Hệ thống khởi tạo và tiến hành đọc dữ liệu định kỳ từ **BMI160** và **GPS6N**.
2. **Xử lý Dữ liệu:** Vi điều khiển tiến hành tính toán số liệu cần thiết (vận tốc, tọa độ, góc nghiêng,...).
3. **Phân nhánh Dữ liệu:** 
   - *Luồng 1:* Ghi dữ liệu thô/đã xử lý vào **Thẻ nhớ SD** (dùng cho việc trích xuất hộp đen sau này).
   - *Luồng 2:* Gửi trực tiếp thông tin lên **Webserver** qua **MQTT** bằng module SIM.
4. **Phản hồi Server:** Module SIM lắng nghe các topic từ MQTT. Khi có bản tin yêu cầu tải dữ liệu từ Webserver, RP2040 mở file trong thẻ nhớ, đọc các gói dữ liệu theo mốc thời gian hoặc file chỉ định và gửi lên theo đường MQTT.

## Hướng dẫn cài đặt và sử dụng

components/
├── alogithm_bmi160/      # Thuật toán tính Pitch/Roll/Lực G từ dữ liệu thô
├── alogithm_neo6m/       # Thuật toán parse chuỗi NMEA thô ra tọa độ/vận tốc
├── bmi160/               # Driver SPI đọc/ghi thanh ghi thô BMI160
├── neo6m/                # Driver UART cấu hình và nhận chuỗi thô từ GPS
├── xpt2046/              # [BỔ SUNG] Driver đọc tọa độ cảm ứng màn hình
├── tft/                  # Driver hiển thị ILI9341 (Vẽ pixel, hình chữ nhật, chữ)
├── sdcard/               # Driver FatFs để giao tiếp thẻ nhớ SPI
├── simA7680C/            # Driver quản lý tập lệnh AT cho SIM
├── my_mqtt/              # Trình quản lý giao thức và đóng gói bản tin MQTT
├── buzzer/               # Driver điều khiển còi báo bíp bíp
└── shared_data/          # [BỔ SUNG] Quản lý cấu trúc dữ liệu chung & Mutex giữa các Task


## Tiến độ dự án
Hiện tại dự án chỉ mới khởi tạo khung dự án.
- Tiếp theo: kiểm tra kết nối của sim và gửi 1 mẩu tin nhỏ lên mqtt

KHUNG GIAO DIỆN CHUNG (FIXED TOP BAR & BOTTOM TABS)
Thành phần này luôn xuất hiện ở mọi trang để người dùng bao quát được trạng thái cốt lõi:

Top Bar (Cao 30px): [Icon Thẻ nhớ] [Icon Sóng SIM] | [Thời gian rút gọn hh:mm] | [Dung lượng Pin %]

Bottom Tabs (Cao 35px): Chia làm 3 nút cảm ứng lớn nằm ngang ở đáy màn hình: [ PAGE 1 ]  [ PAGE 2 ]  [ PAGE 3 ] (Tab nào đang chọn sẽ đổi màu nền sáng lên).

📄 CHI TIẾT LAYOUT TỪNG TRANG (Diện tích hiển thị: 240 x 255px)
TRANG 1: ĐỊNH VỊ VÀ THỜI GIAN VỆ TINH (GPS & UTC TIME)
Trang này tập trung vào dữ liệu không gian và thời gian thô từ module Neo-6M. Thích hợp để người điều hành kiểm tra xem thiết bị đã bắt được sóng vệ tinh "khỏe" chưa trước khi xuất phát.

Plaintext
+-----------------------------------+
| [SD] [SIM:OK]     21:35     [85%] | <- Top Bar
+-----------------------------------+
|          GPS DIAGNOSTICS          |
|                                   |
|  * SAT IN USE: 08 Satellites      | <- Số lượng vệ tinh kết nối
|  * LAT (Vĩ độ):                   |
|    21.054321° N                   |
|  * LON (Kinh độ):                 |
|    105.735462° E                  |
|                                   |
|  * UTC TIME (Đồng bộ):            |
|    2026-06-23 14:35:12            | <- Giờ quốc tế chuẩn từ GPS
|                                   |
+-----------------------------------+
| [ PAGE 1 ] |  PAGE 2  |  PAGE 3   | <- Bottom Tabs
+-----------------------------------+
TRANG 2: HÀNH TRÌNH VÀ CHUYỂN ĐỘNG (DYNAMICS & MOTION)
Đây là trang động chính của Hộp đen. Nó kết hợp dữ liệu cơ học của BMI160 và dữ liệu di chuyển từ GPS. Nên sử dụng các khối màu hoặc thanh tiến trình (Progress Bar) nhỏ để dễ quan sát khi thiết bị đang di chuyển.

Plaintext
+-----------------------------------+
| [SD] [SIM:OK]     21:35     [85%] |
+-----------------------------------+
|         MOTION & TELEMETRY        |
|                                   |
|  SPEED (Vận tốc):  65.5 km/h      | <- Lấy từ GPS
|  HEADING (Hướng):  180° (SOUTH)   | <- Hướng di chuyển GPS
|                                   |
|  [ BMI160 IMU SENSOR ]            |
|  * Pitch (Nghiêng dọc):  +12.4°   |
|  * Roll  (Nghiêng ngang): -2.5°   |
|  * G-Force (Lực G):       1.05 G  |
|                                   |
|    X: [||||||....]  Y: [||||.....] | <- Thanh bar hiển thị trực quan
+-----------------------------------+
|   PAGE 1   | [ PAGE 2 ] |  PAGE 3   |
+-----------------------------------+
TRANG 3: HỆ THỐNG LƯU TRỮ VÀ TÀI NGUYÊN (STORAGE & SYSTEM)
Trang quản lý kỹ thuật, giúp người dùng kiểm tra "sức khỏe" của bộ nhớ SD Card và tiến trình lưu trữ dữ liệu xem có xảy ra lỗi ghi file không.

Plaintext
+-----------------------------------+
| [SD] [SIM:OK]     21:35     [85%] |
+-----------------------------------+
|          STORAGE STATUS           |
|                                   |
|  * SD CARD: DETECTED (Đã nhận)    |
|  * FILE SYSTEM: FAT32             |
|                                   |
|  * CAPACITY (Dung lượng):         |
|    [||||||||||||||||||.......]    | <- Thanh dung lượng đã dùng
|    Used: 4.2 GB / Free: 11.8 GB   |
|                                   |
|  * STATUS (Trạng thái):           |
|    >> LOGGING... (Đang ghi file)  | <- Nhấp nháy chữ đỏ hoặc xanh
|    Filename: log_20260623.csv     |
+-----------------------------------+
|   PAGE 1   |  PAGE 2  | [ PAGE 3 ] |
+-----------------------------------+


Phản hồi trạng thái khởi động (Boot Status Feedback)
Khi bạn cấp nguồn cho Hộp đen, hệ thống cần thời gian để kiểm tra các ngoại vi (SD Card, BMI160, GPS, SIM). Còi sẽ phát ra âm thanh để báo hiệu:

Kêu 2 tiếng "bíp bíp" ngắn: Toàn bộ hệ thống khởi động hoàn hảo, thẻ nhớ sẵn sàng, cảm biến hoạt động tốt.

Kêu 1 tiếng "biiiiíp" dài liên tục: Hệ thống gặp lỗi nghiêm trọng lúc khởi động (ví dụ: Không tìm thấy thẻ nhớ SD hoặc lỗi giao tiếp cảm biến BMI160). Điều này giúp bạn biết ngay để kiểm tra phần cứng mà không cần nhìn màn hình.


Trong kịch bản thiết bị bay gặp sự cố và rơi xuống địa hình khuất (bụi cây, rãnh sâu), còi sẽ chuyển sang chế độ Phát tín hiệu định vị. Nó sẽ tự động kêu theo chu kỳ (ví dụ: cứ 3 giây kêu 1 tiếng lớn) để đội cứu hộ hoặc chính bạn có thể dễ dàng lần theo âm thanh và tìm lại Hộp đen giữa thực địa.


tiến trình dự án
Giai đoạn 1 — Đang làm (kết nối mạng)
├── ✅ Khởi tạo SIM7080G
├── 🔄 Đăng ký mạng CAT-M (đang debug)
└── ⬜ Kết nối GPRS/data

Giai đoạn 2 — Kết nối MQTT
├── ⬜ Mở TCP tới MQTT Broker
├── ⬜ Publish dữ liệu test
└── ⬜ Subscribe nhận lệnh

Giai đoạn 3 — Tích hợp cảm biến
├── ✅ Đọc GPS Neo-6M
├── ✅ Đọc IMU BMI160
└── ⬜ Đóng gói JSON và gửi

Giai đoạn 4 — Dashboard
├── ⬜ Setup MQTT Broker
├── ⬜ Hiển thị bản đồ real-time
└── ⬜ Giao diện gửi lệnh xuống


hướng dẫn nạp code: 
.\build.ps1
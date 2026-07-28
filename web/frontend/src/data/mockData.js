import { ShieldAlert, Send, MapPin, Radio } from "lucide-react";

export const MOCK = {
    gps: { lat: 21.054321, lon: 105.735462, sats: 8, utc: "2026-07-02 09:42:11", speed: 65.5, heading: 180 },
    motion: {
        pitch: 12.4, roll: -2.5, heading: 214, gforce: 1.05,
        accel: [{ axis: "X", g: 0.62 }, { axis: "Y", g: 0.18 }, { axis: "Z", g: 0.95 }],
        gyro: [{ axis: "X", dps: 12 }, { axis: "Y", dps: -4 }, { axis: "Z", dps: 2 }],
    },
    storage: {
        usedGB: 4.2, totalGB: 16, filename: "log_20260702.csv", writeRate: "12 KB/s",
        files: [
            { name: "log_20260702.csv", size: "18.4 MB", date: "02/07/2026 09:40" },
            { name: "log_20260701.csv", size: "22.1 MB", date: "01/07/2026 07:15" },
            { name: "log_20260630.csv", size: "9.8 MB", date: "30/06/2026 16:02" },
        ],
    },
    system: {
        firmware: "v0.1", board: "YD_RP2040_RP2_B2", uptime: "02:14:33", mqtt: true, broker: "broker.hivemq.com:1883",
        pins: [
            { bus: "I2C0", dev: "BMI160 IMU", pins: "GP20 / GP21" },
            { bus: "UART0", dev: "Neo-6M GPS", pins: "GP0 / GP1" },
            { bus: "UART1", dev: "SIM A7680C", pins: "GP4 / GP5" },
            { bus: "SPI1", dev: "TFT ILI9341", pins: "GP12 / GP15 / GP14" },
            { bus: "SPI0", dev: "XPT2046 + SD", pins: "GP16-19 / GP22 / GP23" },
            { bus: "GPIO", dev: "Còi báo (Buzzer)", pins: "GP2" },
        ],
    },
    history: [
        { date: "02/07/2026", duration: "48 phút", distance: "31.2 km", maxSpeed: "78 km/h", status: "normal" },
        { date: "01/07/2026", duration: "1h 12p", distance: "54.6 km", maxSpeed: "82 km/h", status: "normal" },
        { date: "29/06/2026", duration: "22 phút", distance: "14.0 km", maxSpeed: "65 km/h", status: "alert" },
        { date: "27/06/2026", duration: "36 phút", distance: "20.5 km", maxSpeed: "70 km/h", status: "normal" },
    ],
    alerts: [
        { time: "09:41", msg: "Đã khóa vị trí GPS — 8 vệ tinh", level: "ok" },
        { time: "09:38", msg: "Kết nối MQTT tới broker.hivemq.com thành công", level: "ok" },
        { time: "09:30", msg: "Bắt đầu ghi log_20260702.csv", level: "ok" },
        { time: "08:55", msg: "Tín hiệu SIM yếu trong 12 giây", level: "warn" },
    ],
};

export const TEST_ITEMS = [
    { key: "gps", label: "Định vị GPS", desc: "Kiểm tra khóa vệ tinh Neo-6M" },
    { key: "imu", label: "Cảm biến IMU", desc: "Kiểm tra BMI160 (I2C0)" },
    { key: "sd", label: "Thẻ nhớ SD", desc: "Kiểm tra ghi/đọc thẻ nhớ" },
    { key: "sim", label: "SIM & MQTT", desc: "Kiểm tra kết nối A7680C" },
    { key: "buzzer", label: "Còi báo", desc: "Kiểm tra còi cảnh báo (GP2)" },
];

export const ALERT_TYPES = [
    { type: "emergency", icon: ShieldAlert, label: "Cảnh báo khẩn cấp", tone: "bad" },
    { type: "support", icon: Send, label: "Yêu cầu hỗ trợ mặt đất", tone: "warn" },
    { type: "position", icon: MapPin, label: "Báo cáo vị trí hiện tại", tone: "neutral" },
    { type: "ping", icon: Radio, label: "Kiểm tra tín hiệu", tone: "neutral" },
];

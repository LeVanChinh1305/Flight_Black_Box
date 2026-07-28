import React, { useState } from "react";
import {
    Settings2, Send, Radio, Activity, Circle, Loader2, CheckCircle2, XCircle,
    ClipboardCheck, Rocket, StopCircle, RefreshCw
} from "lucide-react";
import { C, FONT_MONO } from "../../constants/theme";
import { TEST_ITEMS, ALERT_TYPES } from "../../data/mockData";
import { Card } from "../common/Card";
import { SectionTitle } from "../common/SectionTitle";
import { StatusPill } from "../common/StatusPill";
import { StepHeader } from "../common/StepHeader";

function TestItemRow({ item, status, onRun }) {
    const icons = { idle: Circle, testing: Loader2, pass: CheckCircle2, fail: XCircle };
    const colors = { idle: C.inkFaint, testing: C.teal, pass: C.green, fail: C.red };
    const labels = { idle: "Chưa kiểm tra", testing: "Đang kiểm tra…", pass: "Đạt", fail: "Lỗi" };
    const Icon = icons[status];
    return (
        <div className="flex items-center justify-between py-2.5" style={{ borderBottom: `1px solid ${C.border}` }}>
            <div>
                <div className="text-xs font-medium" style={{ color: C.ink }}>{item.label}</div>
                <div className="text-[11px]" style={{ color: C.inkFaint }}>{item.desc}</div>
            </div>
            <div className="flex items-center gap-3">
                <div className="flex items-center gap-1.5">
                    <Icon size={13} style={{ color: colors[status] }} className={status === "testing" ? "animate-spin" : ""} />
                    <span className="text-[11px]" style={{ color: colors[status], fontFamily: FONT_MONO }}>{labels[status]}</span>
                </div>
                <button
                    onClick={onRun}
                    disabled={status === "testing"}
                    className="text-[11px] font-medium px-2.5 py-1 rounded-md hover:bg-slate-50 transition-colors"
                    style={{ border: `1px solid ${C.border}`, color: C.inkSoft, background: C.surface }}
                >
                    Kiểm tra
                </button>
            </div>
        </div>
    );
}

export function ControlSection({ flightState, setFlightState }) {
    const [tests, setTests] = useState({ gps: "idle", imu: "idle", sd: "idle", sim: "idle", buzzer: "idle" });
    const [setup, setSetup] = useState({
        filename: "log_20260702_flight.csv",
        pilot: "",
        aircraftId: "YD-RP2040-01",
        notes: "",
    });
    const [alertLog, setAlertLog] = useState([]);
    const [autoTrigger, setAutoTrigger] = useState(false);

    const runTest = (key) => {
        setTests((t) => ({ ...t, [key]: "testing" }));
        setTimeout(() => {
            setTests((t) => ({ ...t, [key]: "pass" }));
        }, 900 + Math.random() * 500);
    };
    const runAllTests = () => TEST_ITEMS.forEach((it) => runTest(it.key));

    const allPass = Object.values(tests).every((v) => v === "pass");
    const testsDone = allPass;
    const setupDone = setup.filename.trim() !== "" && setup.pilot.trim() !== "";
    const canStart = testsDone && setupDone && flightState !== "flying";

    const nowStr = () => new Date().toLocaleTimeString("vi-VN", { hour: "2-digit", minute: "2-digit" });

    const startFlight = () => {
        if (!canStart) return;
        setFlightState("flying");
        setAlertLog((log) => [{ time: nowStr(), msg: `Bắt đầu ghi log: ${setup.filename}`, level: "ok" }, ...log]);
    };
    const endFlight = () => {
        setFlightState("ended");
        setAlertLog((log) => [{ time: nowStr(), msg: "Đã kết thúc chuyến bay — ngừng ghi log", level: "ok" }, ...log]);
    };
    const newFlight = () => {
        setFlightState("idle");
        setTests({ gps: "idle", imu: "idle", sd: "idle", sim: "idle", buzzer: "idle" });
        setAlertLog([]);
    };
    const sendAlert = (item) => {
        setAlertLog((log) => [{ time: nowStr(), msg: `Đã gửi: ${item.label}`, level: item.tone === "bad" ? "warn" : "ok" }, ...log]);
    };

    const flightLabel = {
        idle: "Chưa sẵn sàng", ready: "Sẵn sàng cất cánh", flying: "Đang bay — đang ghi log", ended: "Đã kết thúc chuyến bay",
    }[flightState] || "Chưa sẵn sàng";
    const flightTone = flightState === "flying" ? "ok" : flightState === "ended" ? "warn" : "neutral";

    return (
        <div>
            <SectionTitle eyebrow="Điều khiển" title="Điều khiển chuyến bay" />

            <Card style={{ marginBottom: 16, background: C.navySoft, border: `1px solid ${C.navySoft}` }}>
                <div className="flex items-center gap-2 mb-2">
                    <Settings2 size={15} style={{ color: C.navy }} />
                    <span className="text-sm font-semibold" style={{ color: C.navy }}>Hộp đen nhận lệnh bắt đầu bằng cách nào?</span>
                </div>
                <p className="text-xs mb-3" style={{ color: C.inkSoft }}>
                    Thiết bị chỉ bắt đầu ghi log khi nhận được một trong các tín hiệu sau — bảng điều khiển này thực hiện đường tín hiệu điều khiển từ xa (1):
                </p>
                <div className="flex flex-wrap gap-2">
                    <StatusPill icon={Send} label="Lệnh từ xa qua MQTT/SIM (dashboard này)" tone="neutral" />
                    <StatusPill icon={Radio} label="Nút vật lý trên thiết bị" tone="neutral" />
                    <StatusPill icon={Activity} label="Tự động khi tốc độ vượt ngưỡng" tone="neutral" />
                </div>
            </Card>

            <Card style={{ marginBottom: 16 }}>
                <StepHeader number="1" title="Kiểm tra hệ thống trước bay" done={allPass} />
                <div>
                    {TEST_ITEMS.map((it) => (
                        <TestItemRow key={it.key} item={it} status={tests[it.key]} onRun={() => runTest(it.key)} />
                    ))}
                </div>
                <button
                    onClick={runAllTests}
                    className="mt-4 flex items-center gap-2 text-xs font-medium px-3 py-2 rounded-lg hover:opacity-90 transition-opacity"
                    style={{ background: C.navy, color: "#fff" }}
                >
                    <ClipboardCheck size={14} /> Chạy tất cả kiểm tra
                </button>
            </Card>

            <Card style={{ marginBottom: 16 }}>
                <StepHeader number="2" title="Thiết lập trước chuyến bay" done={setupDone} />
                <div className="grid grid-cols-2 gap-4">
                    <div>
                        <label className="text-[11px]" style={{ color: C.inkFaint }}>Tên tệp log</label>
                        <input
                            value={setup.filename}
                            onChange={(e) => setSetup((s) => ({ ...s, filename: e.target.value }))}
                            className="w-full mt-1 px-3 py-2 rounded-lg text-xs outline-none focus:border-teal-500"
                            style={{ border: `1px solid ${C.border}`, fontFamily: FONT_MONO, color: C.ink }}
                            placeholder="log_YYYYMMDD_ten_chuyen_bay.csv"
                        />
                    </div>
                    <div>
                        <label className="text-[11px]" style={{ color: C.inkFaint }}>Người vận hành / Phi công</label>
                        <input
                            value={setup.pilot}
                            onChange={(e) => setSetup((s) => ({ ...s, pilot: e.target.value }))}
                            className="w-full mt-1 px-3 py-2 rounded-lg text-xs outline-none focus:border-teal-500"
                            style={{ border: `1px solid ${C.border}`, color: C.ink }}
                            placeholder="Nhập tên"
                        />
                    </div>
                    <div>
                        <label className="text-[11px]" style={{ color: C.inkFaint }}>Mã máy bay / thiết bị</label>
                        <input
                            value={setup.aircraftId}
                            onChange={(e) => setSetup((s) => ({ ...s, aircraftId: e.target.value }))}
                            className="w-full mt-1 px-3 py-2 rounded-lg text-xs outline-none focus:border-teal-500"
                            style={{ border: `1px solid ${C.border}`, fontFamily: FONT_MONO, color: C.ink }}
                        />
                    </div>
                    <div className="flex items-center gap-2" style={{ marginTop: 20 }}>
                        <input
                            type="checkbox"
                            id="autoTrigger"
                            checked={autoTrigger}
                            onChange={(e) => setAutoTrigger(e.target.checked)}
                            className="rounded cursor-pointer"
                        />
                        <label htmlFor="autoTrigger" className="text-xs cursor-pointer" style={{ color: C.inkSoft }}>
                            Tự động bắt đầu khi tốc độ &gt; 5 km/h
                        </label>
                    </div>
                    <div style={{ gridColumn: "span 2" }}>
                        <label className="text-[11px]" style={{ color: C.inkFaint }}>Ghi chú chuyến bay</label>
                        <input
                            value={setup.notes}
                            onChange={(e) => setSetup((s) => ({ ...s, notes: e.target.value }))}
                            className="w-full mt-1 px-3 py-2 rounded-lg text-xs outline-none focus:border-teal-500"
                            style={{ border: `1px solid ${C.border}`, color: C.ink }}
                            placeholder="Tùy chọn"
                        />
                    </div>
                </div>
            </Card>

            <Card style={{ marginBottom: 16 }}>
                <StepHeader number="3" title="Ra tín hiệu bay" done={flightState === "flying" || flightState === "ended"} />
                <div className="flex items-center justify-between">
                    <div>
                        <div className="text-xs" style={{ color: C.inkSoft }}>Trạng thái hiện tại</div>
                        <div className="text-base font-semibold" style={{ color: C.ink }}>{flightLabel}</div>
                        {!canStart && flightState !== "flying" && (
                            <div className="text-[11px] mt-1" style={{ color: C.amber }}>
                                {!testsDone ? "Cần hoàn tất kiểm tra hệ thống ở Bước 1" : "Cần nhập tên tệp và người vận hành ở Bước 2"}
                            </div>
                        )}
                    </div>
                    <StatusPill icon={Rocket} label={flightState.toUpperCase()} tone={flightTone} />
                </div>
                <div className="flex items-center gap-3 mt-4">
                    {flightState !== "flying" ? (
                        <button
                            onClick={startFlight}
                            disabled={!canStart}
                            className="flex items-center gap-2 text-sm font-semibold px-5 py-3 rounded-xl transition-all"
                            style={{ background: canStart ? C.green : C.border, color: canStart ? "#fff" : C.inkFaint, cursor: canStart ? "pointer" : "not-allowed" }}
                        >
                            <Rocket size={16} /> Bắt đầu chuyến bay
                        </button>
                    ) : (
                        <button
                            onClick={endFlight}
                            className="flex items-center gap-2 text-sm font-semibold px-5 py-3 rounded-xl hover:opacity-90 transition-opacity"
                            style={{ background: C.red, color: "#fff" }}
                        >
                            <StopCircle size={16} /> Kết thúc chuyến bay
                        </button>
                    )}
                    {flightState === "ended" && (
                        <button
                            onClick={newFlight}
                            className="flex items-center gap-2 text-xs font-medium px-4 py-2.5 rounded-xl hover:bg-slate-50 transition-colors"
                            style={{ border: `1px solid ${C.border}`, color: C.inkSoft }}
                        >
                            <RefreshCw size={13} /> Thiết lập chuyến bay mới
                        </button>
                    )}
                </div>
            </Card>

            <Card>
                <StepHeader number="4" title="Gửi cảnh báo & yêu cầu khi bay" done={false} />
                <div className="text-[11px] mb-3" style={{ color: C.inkFaint }}>
                    Gửi qua MQTT/SIM tới trạm mặt đất — khả dụng bất kỳ lúc nào, nên dùng trong khi đang bay
                </div>
                <div className="grid grid-cols-4 gap-2 mb-4">
                    {ALERT_TYPES.map((a) => (
                        <button
                            key={a.type}
                            onClick={() => sendAlert(a)}
                            className="flex flex-col items-center gap-1.5 text-center py-3 rounded-lg hover:bg-slate-50 transition-colors"
                            style={{ border: `1px solid ${C.border}`, background: C.surface }}
                        >
                            <a.icon size={16} style={{ color: a.tone === "bad" ? C.red : a.tone === "warn" ? C.amber : C.teal }} />
                            <span className="text-[11px] font-medium" style={{ color: C.inkSoft }}>{a.label}</span>
                        </button>
                    ))}
                </div>
                {alertLog.length > 0 && (
                    <div className="space-y-2 pt-2" style={{ borderTop: `1px solid ${C.border}` }}>
                        {alertLog.slice(0, 6).map((a, i) => (
                            <div key={i} className="flex items-center gap-2">
                                <div className="rounded-full flex-shrink-0" style={{ width: 6, height: 6, background: a.level === "ok" ? C.green : C.amber }} />
                                <span className="text-xs" style={{ color: C.ink }}>{a.msg}</span>
                                <span className="text-[11px] ml-auto" style={{ color: C.inkFaint, fontFamily: FONT_MONO }}>{a.time}</span>
                            </div>
                        ))}
                    </div>
                )}
            </Card>
        </div>
    );
}

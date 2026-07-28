import React from "react";
import {
    Rocket, Satellite, Database, Battery, Volume2, RefreshCw, AlertTriangle
} from "lucide-react";
import { C, FONT_MONO } from "../../constants/theme";
import { MOCK } from "../../data/mockData";
import { Card } from "../common/Card";
import { SectionTitle } from "../common/SectionTitle";
import { StatCard } from "../common/StatCard";

export function OverviewSection({ buzzerStatus, runBuzzerTest, flightState }) {
    const flightLabel = {
        idle: "Chưa thiết lập",
        ready: "Sẵn sàng cất cánh",
        flying: "Đang bay — đang ghi log",
        ended: "Đã kết thúc chuyến bay",
    }[flightState] || "Chưa thiết lập";

    const flightTone = flightState === "flying" ? "ok" : flightState === "ready" ? "neutral" : flightState === "ended" ? "warn" : "neutral";

    return (
        <div>
            <SectionTitle eyebrow="Tổng quan" title="Trạng thái thiết bị trực tiếp" />
            <div className="grid grid-cols-4 gap-4">
                <StatCard icon={Rocket} label="Trạng thái chuyến bay" value={flightState === "flying" ? "ĐANG BAY" : flightState === "ready" ? "SẴN SÀNG" : flightState === "ended" ? "KẾT THÚC" : "CHỜ"} sub={flightLabel} tone={flightTone} />
                <StatCard icon={Satellite} label="Định vị GPS" value="8 vệ tinh" sub="Đã khóa vị trí" tone="ok" />
                <StatCard icon={Database} label="Thẻ nhớ SD" value="4.2 / 16 GB" sub={flightState === "flying" ? "Đang ghi log" : "Sẵn sàng ghi"} tone="ok" />
                <StatCard icon={Battery} label="Mức pin" value="82%" sub="Còn khoảng 5h 40p" tone="neutral" />
            </div>

            <div className="grid grid-cols-3 gap-4 mt-6">
                <Card style={{ gridColumn: "span 2" }}>
                    <div className="flex items-center justify-between mb-4">
                        <div className="flex items-center gap-2">
                            <Volume2 size={16} style={{ color: C.teal }} />
                            <span className="text-sm font-semibold" style={{ color: C.ink }}>Kiểm tra còi báo động</span>
                        </div>
                        <span className="text-xs px-2 py-1 rounded-full" style={{ background: C.navySoft, color: C.navy, fontFamily: FONT_MONO }}>GP2</span>
                    </div>
                    <div className="grid grid-cols-4 gap-2 mb-4">
                        {[
                            { label: "Bíp ngắn", cmd: "Đang kêu 1 tiếng bíp..." },
                            { label: "Boot OK", cmd: "Boot OK — 2 tiếng bíp" },
                            { label: "Boot Error", cmd: "Boot Error — 1 tiếng dài" },
                            { label: "Định vị", cmd: "Chế độ định vị — lặp 3s" },
                        ].map((b) => (
                            <button
                                key={b.label}
                                onClick={() => runBuzzerTest(b.cmd)}
                                className="text-xs font-medium py-2 rounded-lg hover:bg-slate-50 transition-colors"
                                style={{ border: `1px solid ${C.border}`, color: C.inkSoft, background: C.surface }}
                            >
                                {b.label}
                            </button>
                        ))}
                    </div>
                    <div className="flex items-center gap-2 px-3 py-2 rounded-lg" style={{ background: C.bg }}>
                        <RefreshCw size={12} style={{ color: C.teal }} />
                        <span className="text-xs" style={{ color: C.inkSoft, fontFamily: FONT_MONO }}>{buzzerStatus}</span>
                    </div>
                </Card>

                <Card>
                    <div className="flex items-center gap-2 mb-4">
                        <AlertTriangle size={16} style={{ color: C.teal }} />
                        <span className="text-sm font-semibold" style={{ color: C.ink }}>Cảnh báo gần đây</span>
                    </div>
                    <div className="space-y-3">
                        {MOCK.alerts.map((a, i) => (
                            <div key={i} className="flex items-start gap-2">
                                <div className="mt-1 rounded-full flex-shrink-0" style={{ width: 6, height: 6, background: a.level === "ok" ? C.green : C.amber }} />
                                <div>
                                    <div className="text-xs" style={{ color: C.ink }}>{a.msg}</div>
                                    <div className="text-[11px]" style={{ color: C.inkFaint, fontFamily: FONT_MONO }}>{a.time}</div>
                                </div>
                            </div>
                        ))}
                    </div>
                </Card>
            </div>
        </div>
    );
}

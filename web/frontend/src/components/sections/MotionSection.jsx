import React from "react";
import { Plane, CheckCircle2, AlertTriangle, Gauge, Navigation, Compass } from "lucide-react";
import { BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from "recharts";
import { C } from "../../constants/theme";
import { MOCK } from "../../data/mockData";
import { attitudeStatus } from "../../utils/flightUtils";
import { Card } from "../common/Card";
import { SectionTitle } from "../common/SectionTitle";
import { StatusPill } from "../common/StatusPill";
import { ReadoutRow } from "../common/ReadoutRow";
import { TiltBar } from "../common/TiltBar";
import { Attitude3D } from "../3d/Attitude3D";

export function MotionSection() {
    const m = MOCK.motion;
    const status = attitudeStatus(m.pitch, m.roll);
    const pitchTone = Math.abs(m.pitch) > 35 ? "bad" : Math.abs(m.pitch) > 20 ? "warn" : "ok";
    const rollTone = Math.abs(m.roll) > 45 ? "bad" : Math.abs(m.roll) > 25 ? "warn" : "ok";

    return (
        <div>
            <SectionTitle eyebrow="Chuyển động" title="Cảm biến quán tính (BMI160)" />

            <div className="grid grid-cols-3 gap-4">
                <Card style={{ gridColumn: "span 2" }}>
                    <div className="flex items-center justify-between mb-1">
                        <div className="flex items-center gap-2">
                            <Plane size={16} style={{ color: C.teal }} />
                            <span className="text-sm font-semibold" style={{ color: C.ink }}>Tư thế máy bay (mô phỏng 3D)</span>
                        </div>
                        <StatusPill icon={status.tone === "ok" ? CheckCircle2 : AlertTriangle} label={status.label} tone={status.tone} />
                    </div>
                    <div className="text-[11px] mb-3" style={{ color: C.inkFaint }}>
                        Mô hình xoay theo dữ liệu pitch, roll và heading thời gian thực từ IMU
                    </div>
                    <div className="flex items-center justify-center rounded-xl overflow-hidden" style={{ background: `linear-gradient(180deg, ${C.tealSoft} 0%, ${C.bg} 100%)`, border: `1px solid ${C.border}` }}>
                        <Attitude3D pitch={m.pitch} roll={m.roll} heading={m.heading} />
                    </div>
                    <div className="flex items-center justify-center gap-6 mt-2">
                        <div className="flex items-center gap-1.5">
                            <div className="rounded-full" style={{ width: 8, height: 8, background: C.teal }} />
                            <span className="text-[11px]" style={{ color: C.inkFaint }}>Mũi máy bay</span>
                        </div>
                        <div className="flex items-center gap-1.5">
                            <div className="rounded-full" style={{ width: 8, height: 8, background: C.navy }} />
                            <span className="text-[11px]" style={{ color: C.inkFaint }}>Cánh & đuôi</span>
                        </div>
                    </div>
                </Card>

                <div className="space-y-4">
                    <Card>
                        <div className="text-xs mb-3" style={{ color: C.inkSoft }}>Chỉ số tư thế tức thời</div>
                        <ReadoutRow icon={Gauge} label="Lực G tổng hợp" hint="Ngưỡng bình thường: 0.8 – 1.3 G" value={m.gforce} unit="G" tone="ok" />
                        <ReadoutRow icon={Plane} label="Góc chúc/ngẩng (Pitch)" hint={m.pitch >= 0 ? "Mũi máy bay hướng lên" : "Mũi máy bay hướng xuống"} value={(m.pitch > 0 ? "+" : "") + m.pitch} unit="°" tone={pitchTone} />
                        <ReadoutRow icon={Compass} label="Góc nghiêng cánh (Roll)" hint={m.roll >= 0 ? "Nghiêng phải" : "Nghiêng trái"} value={(m.roll > 0 ? "+" : "") + m.roll} unit="°" tone={rollTone} />
                        <div style={{ paddingTop: 12 }}>
                            <ReadoutRow icon={Navigation} label="Hướng bay (Heading)" hint="La bàn — 0° = Bắc" value={m.heading} unit="°" tone="neutral" />
                        </div>
                    </Card>
                </div>
            </div>

            <Card style={{ marginTop: 16 }}>
                <div className="text-xs mb-4" style={{ color: C.inkSoft }}>Góc nghiêng chi tiết</div>
                <div className="grid grid-cols-2 gap-8">
                    <TiltBar label="Pitch (nghiêng dọc)" value={m.pitch} tone={pitchTone} />
                    <TiltBar label="Roll (nghiêng ngang)" value={m.roll} tone={rollTone} />
                </div>
            </Card>

            <div className="grid grid-cols-2 gap-4 mt-4">
                <Card>
                    <div className="text-xs mb-1" style={{ color: C.inkSoft }}>Gia tốc kế (Accelerometer)</div>
                    <div className="text-[11px] mb-3" style={{ color: C.inkFaint }}>Đơn vị: g (1g ≈ 9.8 m/s²) — trục X/Y/Z thân máy bay</div>
                    <ResponsiveContainer width="100%" height={140}>
                        <BarChart data={m.accel} layout="vertical" margin={{ left: 8, right: 16 }}>
                            <CartesianGrid horizontal={false} stroke={C.border} />
                            <XAxis type="number" domain={[-1, 1]} tick={{ fontSize: 10, fill: C.inkFaint }} />
                            <YAxis type="category" dataKey="axis" tick={{ fontSize: 11, fill: C.inkSoft }} width={20} />
                            <Tooltip contentStyle={{ fontSize: 11, borderRadius: 8, border: `1px solid ${C.border}` }} />
                            <Bar dataKey="g" fill={C.teal} radius={[0, 4, 4, 0]} barSize={16} />
                        </BarChart>
                    </ResponsiveContainer>
                </Card>
                <Card>
                    <div className="text-xs mb-1" style={{ color: C.inkSoft }}>Con quay hồi chuyển (Gyroscope)</div>
                    <div className="text-[11px] mb-3" style={{ color: C.inkFaint }}>Đơn vị: độ/giây (°/s) — tốc độ xoay quanh từng trục</div>
                    <ResponsiveContainer width="100%" height={140}>
                        <BarChart data={m.gyro} layout="vertical" margin={{ left: 8, right: 16 }}>
                            <CartesianGrid horizontal={false} stroke={C.border} />
                            <XAxis type="number" domain={[-20, 20]} tick={{ fontSize: 10, fill: C.inkFaint }} />
                            <YAxis type="category" dataKey="axis" tick={{ fontSize: 11, fill: C.inkSoft }} width={20} />
                            <Tooltip contentStyle={{ fontSize: 11, borderRadius: 8, border: `1px solid ${C.border}` }} />
                            <Bar dataKey="dps" fill={C.navy} radius={[0, 4, 4, 0]} barSize={16} />
                        </BarChart>
                    </ResponsiveContainer>
                </Card>
            </div>
        </div>
    );
}

import React, { useState, useEffect, useMemo } from "react";
import {
    CheckCircle2, AlertTriangle, X, MapPinned, Gauge, Activity, Navigation, Plane, Play, Pause
} from "lucide-react";
import { LineChart, Line, XAxis, YAxis, ResponsiveContainer, ReferenceDot } from "recharts";
import { C, FONT_MONO } from "../../constants/theme";
import { Card } from "../common/Card";
import { StatusPill } from "../common/StatusPill";
import { ReadoutRow } from "../common/ReadoutRow";
import { generateTrack, formatTime } from "../../utils/flightUtils";

export function FlightReplayModal({ flight, onClose }) {
    const { track, alertIndex, totalSec } = useMemo(() => generateTrack(flight), [flight]);
    const [index, setIndex] = useState(0);
    const [playing, setPlaying] = useState(false);
    const point = track[index] || track[0];

    useEffect(() => {
        if (!playing) return;
        const id = setInterval(() => {
            setIndex((i) => {
                if (i >= track.length - 1) {
                    setPlaying(false);
                    return i;
                }
                return i + 1;
            });
        }, 220);
        return () => clearInterval(id);
    }, [playing, track.length]);

    const pathD = track.map((p, i) => `${i === 0 ? "M" : "L"} ${p.x} ${p.y}`).join(" ");
    const traveledD = track.slice(0, index + 1).map((p, i) => `${i === 0 ? "M" : "L"} ${p.x} ${p.y}`).join(" ");
    const alertPassed = alertIndex >= 0 && index >= alertIndex;

    return (
        <div
            className="fixed inset-0 flex items-center justify-center z-50 animate-fade-in"
            style={{ background: "rgba(20,24,31,0.55)" }}
            onClick={onClose}
        >
            <div
                onClick={(e) => e.stopPropagation()}
                style={{ background: C.surface, borderRadius: 16, width: 780, maxWidth: "92vw", maxHeight: "90vh", overflowY: "auto", border: `1px solid ${C.border}` }}
            >
                <div className="flex items-center justify-between px-6 py-4" style={{ borderBottom: `1px solid ${C.border}` }}>
                    <div>
                        <div className="text-xs" style={{ color: C.teal, fontWeight: 600 }}>XEM LẠI CHUYẾN BAY</div>
                        <div className="text-lg font-semibold" style={{ color: C.ink }}>{flight.date}</div>
                    </div>
                    <div className="flex items-center gap-3">
                        <StatusPill icon={flight.status === "normal" ? CheckCircle2 : AlertTriangle} label={flight.status === "normal" ? "Bình thường" : "Có cảnh báo"} tone={flight.status === "normal" ? "ok" : "warn"} />
                        <button onClick={onClose} className="flex items-center justify-center rounded-lg hover:opacity-80" style={{ width: 30, height: 30, background: C.bg }}>
                            <X size={15} style={{ color: C.inkSoft }} />
                        </button>
                    </div>
                </div>

                <div className="px-6 py-5">
                    <div className="grid grid-cols-3 gap-4">
                        <Card style={{ gridColumn: "span 2", padding: 16 }}>
                            <div className="flex items-center justify-between mb-2">
                                <div className="flex items-center gap-2">
                                    <MapPinned size={14} style={{ color: C.teal }} />
                                    <span className="text-xs font-semibold" style={{ color: C.ink }}>Đường bay đã ghi</span>
                                </div>
                                {alertPassed && <StatusPill icon={AlertTriangle} label="Cảnh báo tại " value={formatTime(track[alertIndex].t)} tone="warn" />}
                            </div>
                            <svg viewBox="0 0 260 160" style={{ width: "100%", height: 200, background: C.tealSoft, borderRadius: 10 }}>
                                <defs>
                                    <pattern id="grid" width="20" height="20" patternUnits="userSpaceOnUse">
                                        <path d="M 20 0 L 0 0 0 20" fill="none" stroke={C.border} strokeWidth="1" />
                                    </pattern>
                                </defs>
                                <rect width="260" height="160" fill="url(#grid)" />
                                <path d={pathD} fill="none" stroke={C.borderStrong} strokeWidth="2" strokeDasharray="3 3" />
                                <path d={traveledD} fill="none" stroke={C.teal} strokeWidth="3" />
                                {alertIndex >= 0 && (
                                    <circle cx={track[alertIndex].x} cy={track[alertIndex].y} r="6" fill={C.redSoft} stroke={C.red} strokeWidth="2" />
                                )}
                                <circle cx={track[0].x} cy={track[0].y} r="4" fill={C.navy} />
                                <g transform={`translate(${point.x} ${point.y}) rotate(${point.heading + 90})`}>
                                    <circle r="9" fill={C.surface} stroke={C.teal} strokeWidth="2" />
                                    <path d="M 0 -5 L 4 4 L 0 2 L -4 4 Z" fill={C.teal} />
                                </g>
                            </svg>
                            <div className="flex items-center justify-between mt-3 text-[11px]" style={{ color: C.inkFaint }}>
                                <span>● Điểm cất cánh</span>
                                <span>— — quãng đường còn lại</span>
                                <span style={{ color: C.teal }}>— đã bay qua</span>
                            </div>
                        </Card>

                        <Card style={{ padding: 16 }}>
                            <div className="text-xs font-semibold mb-3" style={{ color: C.ink }}>Chỉ số tại thời điểm này</div>
                            <ReadoutRow icon={Gauge} label="Tốc độ" value={point.speed} unit="km/h" tone="ok" />
                            <ReadoutRow icon={Activity} label="Độ cao" value={point.altitude} unit="m" tone="ok" />
                            <ReadoutRow icon={Navigation} label="Hướng bay" value={point.heading} unit="°" tone="neutral" />
                            <ReadoutRow icon={Plane} label="Lực G" value={point.gforce} unit="G" tone={point.gforce > 1.3 ? "bad" : "ok"} />
                        </Card>
                    </div>

                    <div className="grid grid-cols-2 gap-4 mt-4">
                        <Card style={{ padding: 16 }}>
                            <div className="text-[11px] mb-2" style={{ color: C.inkSoft }}>Tốc độ theo thời gian (km/h)</div>
                            <ResponsiveContainer width="100%" height={110}>
                                <LineChart data={track} margin={{ left: -20, right: 8, top: 4 }}>
                                    <XAxis dataKey="t" hide />
                                    <YAxis hide />
                                    <Line type="monotone" dataKey="speed" stroke={C.teal} strokeWidth={2} dot={false} />
                                    <ReferenceDot x={point.t} y={point.speed} r={4} fill={C.teal} stroke="#fff" />
                                </LineChart>
                            </ResponsiveContainer>
                        </Card>
                        <Card style={{ padding: 16 }}>
                            <div className="text-[11px] mb-2" style={{ color: C.inkSoft }}>Độ cao theo thời gian (m)</div>
                            <ResponsiveContainer width="100%" height={110}>
                                <LineChart data={track} margin={{ left: -20, right: 8, top: 4 }}>
                                    <XAxis dataKey="t" hide />
                                    <YAxis hide />
                                    <Line type="monotone" dataKey="altitude" stroke={C.navy} strokeWidth={2} dot={false} />
                                    <ReferenceDot x={point.t} y={point.altitude} r={4} fill={C.navy} stroke="#fff" />
                                </LineChart>
                            </ResponsiveContainer>
                        </Card>
                    </div>

                    <Card style={{ marginTop: 16, padding: 16 }}>
                        <div className="flex items-center gap-4">
                            <button
                                onClick={() => setPlaying((p) => !p)}
                                className="flex items-center justify-center rounded-full flex-shrink-0 hover:opacity-90 transition-opacity"
                                style={{ width: 40, height: 40, background: C.navy }}
                            >
                                {playing ? <Pause size={16} color="#fff" /> : <Play size={16} color="#fff" style={{ marginLeft: 2 }} />}
                            </button>
                            <div className="flex-1">
                                <input
                                    type="range"
                                    min={0}
                                    max={track.length - 1}
                                    value={index}
                                    onChange={(e) => { setPlaying(false); setIndex(parseInt(e.target.value, 10)); }}
                                    className="w-full cursor-pointer"
                                    style={{ accentColor: C.teal }}
                                />
                                <div className="flex items-center justify-between mt-1 text-[11px]" style={{ color: C.inkFaint, fontFamily: FONT_MONO }}>
                                    <span>{formatTime(point.t)}</span>
                                    <span>{flight.duration} · {flight.distance}</span>
                                    <span>{formatTime(totalSec)}</span>
                                </div>
                            </div>
                        </div>
                    </Card>
                </div>
            </div>
        </div>
    );
}

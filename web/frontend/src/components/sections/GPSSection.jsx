import React from "react";
import { Satellite, Navigation, Clock } from "lucide-react";
import { C, FONT_MONO } from "../../constants/theme";
import { MOCK } from "../../data/mockData";
import { Card } from "../common/Card";
import { SectionTitle } from "../common/SectionTitle";

export function GPSSection() {
    const g = MOCK.gps;
    return (
        <div>
            <SectionTitle eyebrow="GPS & Vệ tinh" title="Định vị và thời gian vệ tinh" />
            <div className="grid grid-cols-3 gap-4">
                <Card style={{ gridColumn: "span 2" }}>
                    <div className="text-xs font-medium mb-4" style={{ color: C.inkSoft }}>Tọa độ hiện tại</div>
                    <div className="grid grid-cols-2 gap-6">
                        <div>
                            <div className="text-[11px] mb-1" style={{ color: C.inkFaint }}>VĨ ĐỘ (LAT)</div>
                            <div className="text-3xl font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{g.lat}°N</div>
                        </div>
                        <div>
                            <div className="text-[11px] mb-1" style={{ color: C.inkFaint }}>KINH ĐỘ (LON)</div>
                            <div className="text-3xl font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{g.lon}°E</div>
                        </div>
                    </div>
                    <div className="mt-6 relative rounded-xl overflow-hidden" style={{ height: 180, background: C.tealSoft, backgroundImage: `linear-gradient(${C.border} 1px, transparent 1px), linear-gradient(90deg, ${C.border} 1px, transparent 1px)`, backgroundSize: "24px 24px" }}>
                        <div className="absolute" style={{ top: "48%", left: "52%", transform: "translate(-50%,-50%)" }}>
                            <div className="rounded-full animate-ping absolute inset-0" style={{ background: C.teal, opacity: 0.4 }} />
                            <div className="rounded-full relative" style={{ width: 14, height: 14, background: C.teal, border: "3px solid #fff", boxShadow: `0 0 0 3px ${C.teal}33` }} />
                        </div>
                        <span className="absolute bottom-2 right-3 text-[10px]" style={{ color: C.inkFaint, fontFamily: FONT_MONO }}>bản đồ minh họa — demo</span>
                    </div>
                </Card>
                <div className="space-y-4">
                    <Card>
                        <div className="flex items-center gap-2 mb-2"><Satellite size={14} style={{ color: C.teal }} /><span className="text-xs" style={{ color: C.inkSoft }}>Vệ tinh sử dụng</span></div>
                        <div className="text-2xl font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{g.sats}</div>
                    </Card>
                    <Card>
                        <div className="flex items-center gap-2 mb-2"><Navigation size={14} style={{ color: C.teal, transform: `rotate(${g.heading}deg)` }} /><span className="text-xs" style={{ color: C.inkSoft }}>Tốc độ / Hướng</span></div>
                        <div className="text-2xl font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{g.speed} km/h</div>
                        <div className="text-[11px] mt-1" style={{ color: C.inkFaint }}>{g.heading}° — Nam</div>
                    </Card>
                    <Card>
                        <div className="flex items-center gap-2 mb-2"><Clock size={14} style={{ color: C.teal }} /><span className="text-xs" style={{ color: C.inkSoft }}>UTC (đồng bộ GPS)</span></div>
                        <div className="text-base font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{g.utc}</div>
                    </Card>
                </div>
            </div>
        </div>
    );
}

import React, { useState } from "react";
import { PlayCircle } from "lucide-react";
import { C, FONT_MONO } from "../../constants/theme";
import { MOCK } from "../../data/mockData";
import { Card } from "../common/Card";
import { SectionTitle } from "../common/SectionTitle";
import { FlightReplayModal } from "../modals/FlightReplayModal";

export function HistorySection() {
    const [selected, setSelected] = useState(null);
    return (
        <div>
            <SectionTitle eyebrow="Lịch sử" title="Nhật ký chuyến bay" />
            <Card>
                <div className="divide-y" style={{ borderColor: C.border }}>
                    {MOCK.history.map((h, i) => (
                        <div key={i} className="flex items-center justify-between py-3">
                            <div className="flex items-center gap-3">
                                <div className="rounded-full" style={{ width: 8, height: 8, background: h.status === "normal" ? C.green : C.amber }} />
                                <div>
                                    <div className="text-sm font-medium" style={{ color: C.ink }}>{h.date}</div>
                                    <div className="text-[11px]" style={{ color: C.inkFaint }}>{h.status === "normal" ? "Bình thường" : "Có cảnh báo"}</div>
                                </div>
                            </div>
                            <div className="flex items-center gap-6 text-xs" style={{ color: C.inkSoft, fontFamily: FONT_MONO }}>
                                <span>{h.duration}</span>
                                <span>{h.distance}</span>
                                <span>{h.maxSpeed}</span>
                            </div>
                            <button
                                onClick={() => setSelected(i)}
                                className="flex items-center gap-1 text-xs font-medium px-3 py-1.5 rounded-lg hover:bg-teal-50 transition-colors"
                                style={{ color: C.teal, border: `1px solid ${C.border}` }}
                            >
                                <PlayCircle size={13} /> Xem lại
                            </button>
                        </div>
                    ))}
                </div>
            </Card>
            {selected !== null && (
                <FlightReplayModal flight={MOCK.history[selected]} onClose={() => setSelected(null)} />
            )}
        </div>
    );
}

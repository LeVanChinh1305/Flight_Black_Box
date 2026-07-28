import React from "react";
import { FileText } from "lucide-react";
import { C, FONT_MONO } from "../../constants/theme";
import { MOCK } from "../../data/mockData";
import { Card } from "../common/Card";
import { SectionTitle } from "../common/SectionTitle";

export function StorageSection() {
    const s = MOCK.storage;
    const pct = Math.round((s.usedGB / s.totalGB) * 100);
    return (
        <div>
            <SectionTitle eyebrow="Lưu trữ" title="Hệ thống lưu trữ SD Card" />
            <div className="grid grid-cols-3 gap-4">
                <Card style={{ gridColumn: "span 2" }}>
                    <div className="flex items-center justify-between mb-3">
                        <span className="text-xs" style={{ color: C.inkSoft }}>Dung lượng đã sử dụng</span>
                        <span className="text-xs font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{s.usedGB} / {s.totalGB} GB</span>
                    </div>
                    <div className="rounded-full overflow-hidden" style={{ height: 10, background: C.bg, border: `1px solid ${C.border}` }}>
                        <div style={{ width: `${pct}%`, height: "100%", background: C.teal }} />
                    </div>
                    <div className="flex items-center gap-2 mt-4">
                        <div className="rounded-full" style={{ width: 8, height: 8, background: C.green }} />
                        <span className="text-xs" style={{ color: C.ink }}>Đang ghi: {s.filename}</span>
                        <span className="text-[11px] ml-auto" style={{ color: C.inkFaint, fontFamily: FONT_MONO }}>{s.writeRate}</span>
                    </div>
                </Card>
                <Card>
                    <div className="text-xs mb-2" style={{ color: C.inkSoft }}>Hệ thống tệp</div>
                    <div className="text-2xl font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>FAT32</div>
                    <div className="text-[11px] mt-1" style={{ color: C.inkFaint }}>Thẻ nhớ: đã nhận diện</div>
                </Card>
            </div>
            <Card style={{ marginTop: 16 }}>
                <div className="text-xs mb-3" style={{ color: C.inkSoft }}>Tệp log gần đây</div>
                <div className="divide-y" style={{ borderColor: C.border }}>
                    {s.files.map((f, i) => (
                        <div key={i} className="flex items-center justify-between py-2.5">
                            <div className="flex items-center gap-2">
                                <FileText size={14} style={{ color: C.teal }} />
                                <span className="text-xs" style={{ color: C.ink, fontFamily: FONT_MONO }}>{f.name}</span>
                            </div>
                            <div className="flex items-center gap-6">
                                <span className="text-xs" style={{ color: C.inkFaint }}>{f.date}</span>
                                <span className="text-xs" style={{ color: C.inkSoft, fontFamily: FONT_MONO }}>{f.size}</span>
                            </div>
                        </div>
                    ))}
                </div>
            </Card>
        </div>
    );
}

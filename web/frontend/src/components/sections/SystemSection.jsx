import React from "react";
import { Wifi, WifiOff } from "lucide-react";
import { C, FONT_MONO } from "../../constants/theme";
import { MOCK } from "../../data/mockData";
import { Card } from "../common/Card";
import { SectionTitle } from "../common/SectionTitle";

export function SystemSection() {
    const s = MOCK.system;
    return (
        <div>
            <SectionTitle eyebrow="Hệ thống" title="Thông tin thiết bị & kết nối" />
            <div className="grid grid-cols-3 gap-4">
                <Card>
                    <div className="text-xs mb-2" style={{ color: C.inkSoft }}>Bo mạch</div>
                    <div className="text-base font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{s.board}</div>
                    <div className="text-[11px] mt-1" style={{ color: C.inkFaint }}>Firmware {s.firmware}</div>
                </Card>
                <Card>
                    <div className="text-xs mb-2" style={{ color: C.inkSoft }}>Thời gian hoạt động</div>
                    <div className="text-base font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{s.uptime}</div>
                </Card>
                <Card>
                    <div className="flex items-center justify-between">
                        <div>
                            <div className="text-xs mb-2" style={{ color: C.inkSoft }}>Kết nối MQTT</div>
                            <div className="text-base font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{s.mqtt ? "Đã kết nối" : "Mất kết nối"}</div>
                        </div>
                        {s.mqtt ? <Wifi size={18} style={{ color: C.green }} /> : <WifiOff size={18} style={{ color: C.red }} />}
                    </div>
                    <div className="text-[11px] mt-2" style={{ color: C.inkFaint, fontFamily: FONT_MONO }}>{s.broker}</div>
                </Card>
            </div>

            <Card style={{ marginTop: 16 }}>
                <div className="text-xs mb-3" style={{ color: C.inkSoft }}>Bản đồ chân kết nối ngoại vi</div>
                <table className="w-full" style={{ borderCollapse: "collapse" }}>
                    <thead>
                        <tr>
                            {["Bus", "Thiết bị", "Chân GPIO"].map((h) => (
                                <th key={h} className="text-left text-[11px] font-medium pb-2" style={{ color: C.inkFaint, borderBottom: `1px solid ${C.border}` }}>{h}</th>
                            ))}
                        </tr>
                    </thead>
                    <tbody>
                        {s.pins.map((p, i) => (
                            <tr key={i}>
                                <td className="text-xs py-2" style={{ color: C.navy, fontFamily: FONT_MONO, borderBottom: `1px solid ${C.border}` }}>{p.bus}</td>
                                <td className="text-xs py-2" style={{ color: C.ink, borderBottom: `1px solid ${C.border}` }}>{p.dev}</td>
                                <td className="text-xs py-2" style={{ color: C.inkSoft, fontFamily: FONT_MONO, borderBottom: `1px solid ${C.border}` }}>{p.pins}</td>
                            </tr>
                        ))}
                    </tbody>
                </table>
            </Card>
        </div>
    );
}

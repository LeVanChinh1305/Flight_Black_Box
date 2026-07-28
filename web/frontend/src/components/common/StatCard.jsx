import React from "react";
import { Circle } from "lucide-react";
import { Card } from "./Card";
import { C, FONT_MONO } from "../../constants/theme";

export function StatCard({ icon: Icon, label, value, sub, tone = "neutral" }) {
    const tones = { ok: C.green, warn: C.amber, bad: C.red, neutral: C.navy };
    return (
        <Card>
            <div className="flex items-center justify-between mb-3">
                <div className="flex items-center justify-center rounded-lg" style={{ width: 34, height: 34, background: C.tealSoft }}>
                    <Icon size={16} style={{ color: C.teal }} />
                </div>
                <Circle size={8} fill={tones[tone]} color={tones[tone]} />
            </div>
            <div className="text-2xl font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{value}</div>
            <div className="text-xs mt-1" style={{ color: C.inkSoft }}>{label}</div>
            {sub && <div className="text-[11px] mt-2" style={{ color: C.inkFaint }}>{sub}</div>}
        </Card>
    );
}

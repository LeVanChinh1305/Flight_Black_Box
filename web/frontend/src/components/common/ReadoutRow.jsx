import React from "react";
import { C, FONT_MONO } from "../../constants/theme";

export function ReadoutRow({ icon: Icon, label, value, unit, hint, tone = "neutral" }) {
    const tones = { ok: C.green, warn: C.amber, bad: C.red, neutral: C.ink };
    return (
        <div className="flex items-center justify-between py-3" style={{ borderBottom: `1px solid ${C.border}` }}>
            <div className="flex items-center gap-3">
                <div className="flex items-center justify-center rounded-lg" style={{ width: 30, height: 30, background: C.tealSoft }}>
                    <Icon size={14} style={{ color: C.teal }} />
                </div>
                <div>
                    <div className="text-xs" style={{ color: C.inkSoft }}>{label}</div>
                    {hint && <div className="text-[11px]" style={{ color: C.inkFaint }}>{hint}</div>}
                </div>
            </div>
            <div className="text-lg font-semibold" style={{ color: tones[tone] || C.ink, fontFamily: FONT_MONO }}>
                {value}<span className="text-xs ml-1" style={{ color: C.inkFaint }}>{unit}</span>
            </div>
        </div>
    );
}

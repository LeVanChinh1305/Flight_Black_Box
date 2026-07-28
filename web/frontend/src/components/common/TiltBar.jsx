import React from "react";
import { C, FONT_MONO } from "../../constants/theme";

export function TiltBar({ label, value, min = -45, max = 45, tone = "neutral" }) {
    const pct = ((value - min) / (max - min)) * 100;
    const tones = { ok: C.teal, warn: C.amber, bad: C.red, neutral: C.teal };
    return (
        <div>
            <div className="flex items-center justify-between mb-1">
                <span className="text-xs" style={{ color: C.inkSoft }}>{label}</span>
                <span className="text-xs font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{value > 0 ? "+" : ""}{value}°</span>
            </div>
            <div className="relative rounded-full" style={{ height: 8, background: C.bg, border: `1px solid ${C.border}` }}>
                <div className="absolute top-0 bottom-0" style={{ left: "50%", width: 1, background: C.borderStrong }} />
                <div
                    className="absolute top-0 bottom-0 rounded-full transition-all duration-300"
                    style={{
                        left: value >= 0 ? "50%" : `${pct}%`,
                        width: `${Math.abs(pct - 50)}%`,
                        background: tones[tone] || C.teal,
                    }}
                />
            </div>
        </div>
    );
}

import React from "react";
import { C, FONT_MONO } from "../../constants/theme";

export function StatusPill({ icon: Icon, label, value, tone = "neutral" }) {
    const tones = {
        ok: { bg: C.greenSoft, fg: C.green },
        warn: { bg: C.amberSoft, fg: C.amber },
        bad: { bg: C.redSoft, fg: C.red },
        neutral: { bg: C.navySoft, fg: C.navy },
    };
    const t = tones[tone] || tones.neutral;
    return (
        <div className="flex items-center gap-2 px-3 py-1.5 rounded-full" style={{ background: t.bg }}>
            {Icon && <Icon size={14} style={{ color: t.fg }} />}
            <span className="text-xs font-medium" style={{ color: t.fg }}>{label}</span>
            {value && <span className="text-xs font-semibold" style={{ color: t.fg, fontFamily: FONT_MONO }}>{value}</span>}
        </div>
    );
}

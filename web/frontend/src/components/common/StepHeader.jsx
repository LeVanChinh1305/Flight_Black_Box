import React from "react";
import { CheckCircle2 } from "lucide-react";
import { C, FONT_MONO } from "../../constants/theme";

export function StepHeader({ number, title, done }) {
    return (
        <div className="flex items-center gap-3 mb-4">
            <div
                className="flex items-center justify-center rounded-full flex-shrink-0 transition-colors"
                style={{ width: 26, height: 26, background: done ? C.green : C.navy, color: "#fff", fontFamily: FONT_MONO, fontSize: 12, fontWeight: 600 }}
            >
                {done ? <CheckCircle2 size={14} /> : number}
            </div>
            <span className="text-sm font-semibold" style={{ color: C.ink }}>{title}</span>
        </div>
    );
}

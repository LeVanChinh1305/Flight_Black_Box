import React from "react";
import { C } from "../../constants/theme";

export function SectionTitle({ eyebrow, title }) {
    return (
        <div className="mb-6">
            <div className="text-xs font-semibold tracking-wide uppercase mb-1" style={{ color: C.teal }}>{eyebrow}</div>
            <h1 className="text-2xl font-semibold" style={{ color: C.ink }}>{title}</h1>
        </div>
    );
}

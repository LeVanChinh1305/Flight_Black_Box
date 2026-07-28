import React from "react";
import { C } from "../../constants/theme";

export function Card({ children, style, className = "", ...props }) {
    return (
        <div
            style={{ background: C.surface, border: `1px solid ${C.border}`, borderRadius: 12, ...style }}
            className={`p-5 ${className}`}
            {...props}
        >
            {children}
        </div>
    );
}

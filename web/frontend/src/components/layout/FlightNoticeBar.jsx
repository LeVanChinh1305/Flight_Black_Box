import React from "react";
import { Rocket, ClipboardCheck, AlertTriangle, Circle } from "lucide-react";
import { C } from "../../constants/theme";

export function FlightNoticeBar({ flightState, setActiveSection }) {
    const config = {
        flying: { bg: C.greenSoft, fg: C.green, icon: Rocket, text: "Đang có chuyến bay diễn ra — thiết bị đang ghi log telemetry" },
        ready: { bg: C.navySoft, fg: C.navy, icon: ClipboardCheck, text: "Đã sẵn sàng — chưa có chuyến bay nào đang diễn ra" },
        ended: { bg: C.amberSoft, fg: C.amber, icon: AlertTriangle, text: "Không có chuyến bay nào đang diễn ra — chuyến gần nhất đã kết thúc" },
        idle: { bg: C.bg, fg: C.inkSoft, icon: Circle, text: "Không có chuyến bay nào đang diễn ra" },
    }[flightState] || { bg: C.bg, fg: C.inkSoft, icon: Circle, text: "Không có chuyến bay nào đang diễn ra" };
    
    const Icon = config.icon;

    return (
        <div className="flex items-center justify-between px-8 py-2.5" style={{ background: config.bg, borderBottom: `1px solid ${C.border}` }}>
            <div className="flex items-center gap-2">
                <Icon size={14} style={{ color: config.fg }} />
                <span className="text-xs font-medium" style={{ color: config.fg }}>{config.text}</span>
            </div>
            {flightState !== "flying" && (
                <button
                    onClick={() => setActiveSection("control")}
                    className="text-[11px] font-medium hover:opacity-80 transition-opacity"
                    style={{ color: config.fg, textDecoration: "underline" }}
                >
                    Đến Điều khiển chuyến bay
                </button>
            )}
        </div>
    );
}

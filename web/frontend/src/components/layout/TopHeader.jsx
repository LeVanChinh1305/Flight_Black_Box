import React from "react";
import { Rocket, Database, Wifi, Battery } from "lucide-react";
import { C, NAV_ITEMS } from "../../constants/theme";
import { StatusPill } from "../common/StatusPill";

export function TopHeader({ activeSection, flightState }) {
    const currentNavItem = NAV_ITEMS.find((n) => n.id === activeSection);

    return (
        <header className="sticky top-0 z-10 flex items-center justify-between px-8 py-3" style={{ background: "rgba(247,248,250,0.9)", backdropFilter: "blur(6px)", borderBottom: `1px solid ${C.border}` }}>
            <div className="text-sm font-medium" style={{ color: C.inkSoft }}>
                Bảng điều khiển / <span style={{ color: C.ink, fontWeight: 600 }}>{currentNavItem?.label}</span>
            </div>
            <div className="flex items-center gap-2">
                <StatusPill icon={Rocket} label={flightState === "flying" ? "Đang bay" : flightState === "ended" ? "Đã kết thúc" : "Chờ"} tone={flightState === "flying" ? "ok" : "neutral"} />
                <StatusPill icon={Database} label="SD" value="OK" tone="ok" />
                <StatusPill icon={Wifi} label="SIM" value="OK" tone="ok" />
                <StatusPill icon={Battery} label="Pin" value="82%" tone="neutral" />
            </div>
        </header>
    );
}

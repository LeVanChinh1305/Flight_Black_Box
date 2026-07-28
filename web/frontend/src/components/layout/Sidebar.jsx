import React from "react";
import { Radio, ChevronRight } from "lucide-react";
import { C, NAV_ITEMS, FONT_MONO } from "../../constants/theme";

export function Sidebar({ activeSection, setActiveSection }) {
    return (
        <aside className="flex flex-col justify-between flex-shrink-0" style={{ width: 248, background: C.surface, borderRight: `1px solid ${C.border}` }}>
            <div>
                <div className="px-5 pt-6 pb-5" style={{ borderBottom: `1px solid ${C.border}` }}>
                    <div className="flex items-center gap-2">
                        <div className="flex items-center justify-center rounded-lg" style={{ width: 32, height: 32, background: C.navy }}>
                            <Radio size={16} color="#fff" />
                        </div>
                        <div>
                            <div className="text-sm font-semibold" style={{ color: C.ink }}>Flight Black Box</div>
                            <div className="text-[11px]" style={{ color: C.inkFaint, fontFamily: FONT_MONO }}>YD_RP2040_RP2_B2</div>
                        </div>
                    </div>
                </div>
                <nav className="p-3 space-y-1">
                    {NAV_ITEMS.map((item) => {
                        const active = activeSection === item.id;
                        const Icon = item.icon;
                        return (
                            <button
                                key={item.id}
                                onClick={() => setActiveSection(item.id)}
                                className="w-full flex items-center gap-3 px-3 py-2 rounded-lg text-left transition-colors"
                                style={{ background: active ? C.navy : "transparent", color: active ? "#fff" : C.inkSoft }}
                            >
                                <Icon size={16} />
                                <span className="text-sm" style={{ fontWeight: active ? 600 : 400 }}>{item.label}</span>
                                {active && <ChevronRight size={14} className="ml-auto" />}
                            </button>
                        );
                    })}
                </nav>
            </div>
        </aside>
    );
}

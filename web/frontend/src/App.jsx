import React, { useState } from "react";
import { C, FONT_SANS } from "./constants/theme";
import { Sidebar } from "./components/layout/Sidebar";
import { TopHeader } from "./components/layout/TopHeader";
import { FlightNoticeBar } from "./components/layout/FlightNoticeBar";

import { OverviewSection } from "./components/sections/OverviewSection";
import { ControlSection } from "./components/sections/ControlSection";
import { GPSSection } from "./components/sections/GPSSection";
import { MotionSection } from "./components/sections/MotionSection";
import { StorageSection } from "./components/sections/StorageSection";
import { SystemSection } from "./components/sections/SystemSection";
import { HistorySection } from "./components/sections/HistorySection";

export default function App() {
    const [activeSection, setActiveSection] = useState("overview");
    const [buzzerStatus, setBuzzerStatus] = useState("Chờ lệnh (Standby)");
    const [flightState, setFlightState] = useState("idle");

    const runBuzzerTest = (label) => {
        setBuzzerStatus(label);
        setTimeout(() => setBuzzerStatus("Chờ lệnh (Standby)"), 1600);
    };

    return (
        <div className="flex h-screen overflow-hidden" style={{ minHeight: 640, background: C.bg, fontFamily: FONT_SANS }}>
            <Sidebar activeSection={activeSection} setActiveSection={setActiveSection} />

            <main className="flex-1 overflow-y-auto flex flex-col">
                <TopHeader activeSection={activeSection} flightState={flightState} />

                <FlightNoticeBar flightState={flightState} setActiveSection={setActiveSection} />

                <div className="flex-1 px-8 py-8">
                    {activeSection === "overview" && <OverviewSection buzzerStatus={buzzerStatus} runBuzzerTest={runBuzzerTest} flightState={flightState} />}
                    {activeSection === "control" && <ControlSection flightState={flightState} setFlightState={setFlightState} />}
                    {activeSection === "gps" && <GPSSection />}
                    {activeSection === "motion" && <MotionSection />}
                    {activeSection === "storage" && <StorageSection />}
                    {activeSection === "system" && <SystemSection />}
                    {activeSection === "history" && <HistorySection />}
                </div>
            </main>
        </div>
    );
}

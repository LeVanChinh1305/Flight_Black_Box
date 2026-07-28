import {
    LayoutDashboard, MapPin, Activity, HardDrive, Cpu, History, Rocket
} from "lucide-react";

export const C = {
    bg: "#F7F8FA",
    surface: "#FFFFFF",
    border: "#E2E5EA",
    borderStrong: "#CBD1DA",
    ink: "#14181F",
    inkSoft: "#5B6472",
    inkFaint: "#8A93A1",
    navy: "#1B3A5C",
    navySoft: "#EBF0F5",
    teal: "#0E7C86",
    tealSoft: "#E3F3F2",
    amber: "#C77700",
    amberSoft: "#FCF1DE",
    red: "#C23B3B",
    redSoft: "#FBEAEA",
    green: "#1F8A5F",
    greenSoft: "#E7F5EE",
};

export const FONT_MONO = "'IBM Plex Mono', ui-monospace, monospace";
export const FONT_SANS = "'Inter', system-ui, -apple-system, sans-serif";

export const NAV_ITEMS = [
    { id: "overview", label: "Tổng quan", icon: LayoutDashboard },
    { id: "control", label: "Điều khiển chuyến bay", icon: Rocket },
    { id: "gps", label: "GPS & Vệ tinh", icon: MapPin },
    { id: "motion", label: "Chuyển động", icon: Activity },
    { id: "storage", label: "Lưu trữ", icon: HardDrive },
    { id: "system", label: "Hệ thống", icon: Cpu },
    { id: "history", label: "Lịch sử chuyến bay", icon: History },
];

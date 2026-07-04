import React, { useState, useEffect, useRef, useMemo } from "react";
import * as THREE from "three";
import {
  LayoutDashboard, MapPin, Activity, HardDrive, Cpu, History,
  Satellite, Battery, Wifi, WifiOff, AlertTriangle, Radio,
  Navigation, Clock, Database, Volume2, ChevronRight,
  FileText, PlayCircle, RefreshCw, Circle, Plane, CheckCircle2,
  XCircle, Loader2, Send, ShieldAlert, StopCircle, Rocket,
  ClipboardCheck, Compass, Settings2, Gauge, Play, Pause, X, MapPinned
} from "lucide-react";
import {
  BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer,
  LineChart, Line, ReferenceDot
} from "recharts";

const C = {
  bg: "#F7F8FA", surface: "#FFFFFF", border: "#E2E5EA", borderStrong: "#CBD1DA",
  ink: "#14181F", inkSoft: "#5B6472", inkFaint: "#8A93A1",
  navy: "#1B3A5C", navySoft: "#EBF0F5",
  teal: "#0E7C86", tealSoft: "#E3F3F2",
  amber: "#C77700", amberSoft: "#FCF1DE",
  red: "#C23B3B", redSoft: "#FBEAEA",
  green: "#1F8A5F", greenSoft: "#E7F5EE",
};

const FONT_MONO = "'IBM Plex Mono', ui-monospace, monospace";
const FONT_SANS = "'Inter', system-ui, -apple-system, sans-serif";

const NAV_ITEMS = [
  { id: "overview", label: "Tổng quan", icon: LayoutDashboard },
  { id: "control", label: "Điều khiển chuyến bay", icon: Rocket },
  { id: "gps", label: "GPS & Vệ tinh", icon: MapPin },
  { id: "motion", label: "Chuyển động", icon: Activity },
  { id: "storage", label: "Lưu trữ", icon: HardDrive },
  { id: "system", label: "Hệ thống", icon: Cpu },
  { id: "history", label: "Lịch sử chuyến bay", icon: History },
];

const MOCK = {
  gps: { lat: 21.054321, lon: 105.735462, sats: 8, utc: "2026-07-02 09:42:11", speed: 65.5, heading: 180 },
  motion: {
    pitch: 12.4, roll: -2.5, heading: 214, gforce: 1.05,
    accel: [{ axis: "X", g: 0.62 }, { axis: "Y", g: 0.18 }, { axis: "Z", g: 0.95 }],
    gyro: [{ axis: "X", dps: 12 }, { axis: "Y", dps: -4 }, { axis: "Z", dps: 2 }],
  },
  storage: {
    usedGB: 4.2, totalGB: 16, filename: "log_20260702.csv", writeRate: "12 KB/s",
    files: [
      { name: "log_20260702.csv", size: "18.4 MB", date: "02/07/2026 09:40" },
      { name: "log_20260701.csv", size: "22.1 MB", date: "01/07/2026 07:15" },
      { name: "log_20260630.csv", size: "9.8 MB", date: "30/06/2026 16:02" },
    ],
  },
  system: {
    firmware: "v0.1", board: "YD_RP2040_RP2_B2", uptime: "02:14:33", mqtt: true, broker: "broker.hivemq.com:1883",
    pins: [
      { bus: "I2C0", dev: "BMI160 IMU", pins: "GP20 / GP21" },
      { bus: "UART0", dev: "Neo-6M GPS", pins: "GP0 / GP1" },
      { bus: "UART1", dev: "SIM A7680C", pins: "GP4 / GP5" },
      { bus: "SPI1", dev: "TFT ILI9341", pins: "GP12 / GP15 / GP14" },
      { bus: "SPI0", dev: "XPT2046 + SD", pins: "GP16-19 / GP22 / GP23" },
      { bus: "GPIO", dev: "Còi báo (Buzzer)", pins: "GP2" },
    ],
  },
  history: [
    { date: "02/07/2026", duration: "48 phút", distance: "31.2 km", maxSpeed: "78 km/h", status: "normal" },
    { date: "01/07/2026", duration: "1h 12p", distance: "54.6 km", maxSpeed: "82 km/h", status: "normal" },
    { date: "29/06/2026", duration: "22 phút", distance: "14.0 km", maxSpeed: "65 km/h", status: "alert" },
    { date: "27/06/2026", duration: "36 phút", distance: "20.5 km", maxSpeed: "70 km/h", status: "normal" },
  ],
  alerts: [
    { time: "09:41", msg: "Đã khóa vị trí GPS — 8 vệ tinh", level: "ok" },
    { time: "09:38", msg: "Kết nối MQTT tới broker.hivemq.com thành công", level: "ok" },
    { time: "09:30", msg: "Bắt đầu ghi log_20260702.csv", level: "ok" },
    { time: "08:55", msg: "Tín hiệu SIM yếu trong 12 giây", level: "warn" },
  ],
};

function Card({ children, style, ...props }) {
  return (
    <div
      style={{ background: C.surface, border: `1px solid ${C.border}`, borderRadius: 12, ...style }}
      className="p-5"
      {...props}
    >
      {children}
    </div>
  );
}

function StatusPill({ icon: Icon, label, value, tone = "neutral" }) {
  const tones = {
    ok: { bg: C.greenSoft, fg: C.green },
    warn: { bg: C.amberSoft, fg: C.amber },
    bad: { bg: C.redSoft, fg: C.red },
    neutral: { bg: C.navySoft, fg: C.navy },
  };
  const t = tones[tone];
  return (
    <div className="flex items-center gap-2 px-3 py-1.5 rounded-full" style={{ background: t.bg }}>
      <Icon size={14} style={{ color: t.fg }} />
      <span className="text-xs" style={{ color: t.fg }}>{label}</span>
      {value && <span className="text-xs font-semibold" style={{ color: t.fg, fontFamily: FONT_MONO }}>{value}</span>}
    </div>
  );
}

function SectionTitle({ eyebrow, title }) {
  return (
    <div className="mb-6">
      <div className="text-xs font-semibold tracking-wide uppercase mb-1" style={{ color: C.teal }}>{eyebrow}</div>
      <h1 className="text-2xl font-semibold" style={{ color: C.ink }}>{title}</h1>
    </div>
  );
}

function DeviceScreenPreview({ section, flightState }) {
  return (
    <div>
      <div className="text-xs font-medium mb-2" style={{ color: C.inkSoft }}>Xem trước màn hình thiết bị</div>
      <div style={{ width: 168, background: "#0B141C", borderRadius: 14, padding: 8, border: "4px solid #1C2733" }}>
        <div style={{ background: "#0B141C", borderRadius: 6, overflow: "hidden" }}>
          <div className="flex items-center justify-between px-2 py-1" style={{ borderBottom: "1px solid #1C2733" }}>
            <div className="flex items-center gap-1">
              <Database size={9} color="#4FD1B5" />
              <Wifi size={9} color="#4FD1B5" />
            </div>
            <span style={{ fontFamily: FONT_MONO, fontSize: 8, color: "#8FA3B0" }}>09:42</span>
            <span style={{ fontFamily: FONT_MONO, fontSize: 8, color: "#4FD1B5" }}>82%</span>
          </div>
          <div className="p-2" style={{ minHeight: 150 }}>
            {section === "gps" && (
              <div style={{ fontFamily: FONT_MONO, color: "#DCE6EA" }}>
                <div style={{ fontSize: 7, color: "#5F7480" }}>GPS DIAGNOSTICS</div>
                <div style={{ fontSize: 8, marginTop: 6 }}>SAT: 08</div>
                <div style={{ fontSize: 7, color: "#5F7480", marginTop: 4 }}>LAT</div>
                <div style={{ fontSize: 9 }}>21.0543 N</div>
                <div style={{ fontSize: 7, color: "#5F7480", marginTop: 4 }}>LON</div>
                <div style={{ fontSize: 9 }}>105.7355 E</div>
              </div>
            )}
            {section === "motion" && (
              <div style={{ fontFamily: FONT_MONO, color: "#DCE6EA" }}>
                <div style={{ fontSize: 7, color: "#5F7480" }}>MOTION & TELEMETRY</div>
                <div style={{ fontSize: 8, marginTop: 6 }}>65.5 km/h</div>
                <div style={{ fontSize: 7, color: "#5F7480" }}>180° SOUTH</div>
                <div style={{ fontSize: 8, marginTop: 6 }}>Pitch +12.4°</div>
                <div style={{ fontSize: 8 }}>Roll -2.5°</div>
              </div>
            )}
            {section === "storage" && (
              <div style={{ fontFamily: FONT_MONO, color: "#DCE6EA" }}>
                <div style={{ fontSize: 7, color: "#5F7480" }}>STORAGE STATUS</div>
                <div style={{ fontSize: 8, marginTop: 6 }}>4.2 / 16 GB</div>
                <div style={{ fontSize: 7, color: "#4FD1B5", marginTop: 6 }}>● LOGGING...</div>
                <div style={{ fontSize: 7, color: "#5F7480", marginTop: 2 }}>log_20260702.csv</div>
              </div>
            )}
            {section === "control" && (
              <div style={{ fontFamily: FONT_MONO, color: "#DCE6EA" }}>
                <div style={{ fontSize: 7, color: "#5F7480" }}>FLIGHT CONTROL</div>
                <div style={{ fontSize: 9, marginTop: 6 }}>
                  {flightState === "flying" ? "● RECORDING" : flightState === "ready" ? "ARMED" : flightState === "ended" ? "FLIGHT ENDED" : "STANDBY"}
                </div>
                <div style={{ fontSize: 7, color: flightState === "flying" ? "#4FD1B5" : "#5F7480", marginTop: 6 }}>
                  {flightState === "flying" ? "Đang ghi telemetry" : "Chờ lệnh bắt đầu"}
                </div>
              </div>
            )}
            {!["gps", "motion", "storage", "control"].includes(section) && (
              <div style={{ fontFamily: FONT_MONO, color: "#DCE6EA" }}>
                <div style={{ fontSize: 7, color: "#5F7480" }}>FLIGHT BLACK BOX</div>
                <div style={{ fontSize: 8, marginTop: 6 }}>Đang hoạt động</div>
                <div style={{ fontSize: 7, color: "#4FD1B5", marginTop: 6 }}>● READY</div>
              </div>
            )}
          </div>
          <div className="flex" style={{ borderTop: "1px solid #1C2733" }}>
            {["1", "2", "3"].map((n) => (
              <div key={n} className="flex-1 text-center py-1" style={{ fontFamily: FONT_MONO, fontSize: 7, color: "#5F7480" }}>PAGE {n}</div>
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}

function StatCard({ icon: Icon, label, value, sub, tone = "neutral" }) {
  const tones = { ok: C.green, warn: C.amber, bad: C.red, neutral: C.navy };
  return (
    <Card>
      <div className="flex items-center justify-between mb-3">
        <div className="flex items-center justify-center rounded-lg" style={{ width: 34, height: 34, background: C.tealSoft }}>
          <Icon size={16} style={{ color: C.teal }} />
        </div>
        <Circle size={8} fill={tones[tone]} color={tones[tone]} />
      </div>
      <div className="text-2xl font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{value}</div>
      <div className="text-xs mt-1" style={{ color: C.inkSoft }}>{label}</div>
      {sub && <div className="text-[11px] mt-2" style={{ color: C.inkFaint }}>{sub}</div>}
    </Card>
  );
}

function OverviewSection({ buzzerStatus, runBuzzerTest, flightState }) {
  const flightLabel = {
    idle: "Chưa thiết lập", ready: "Sẵn sàng cất cánh", flying: "Đang bay — đang ghi log", ended: "Đã kết thúc chuyến bay",
  }[flightState];
  const flightTone = flightState === "flying" ? "ok" : flightState === "ready" ? "neutral" : flightState === "ended" ? "warn" : "neutral";

  return (
    <div>
      <SectionTitle eyebrow="Tổng quan" title="Trạng thái thiết bị trực tiếp" />
      <div className="grid grid-cols-4 gap-4">
        <StatCard icon={Rocket} label="Trạng thái chuyến bay" value={flightState === "flying" ? "ĐANG BAY" : flightState === "ready" ? "SẴN SÀNG" : flightState === "ended" ? "KẾT THÚC" : "CHỜ"} sub={flightLabel} tone={flightTone} />
        <StatCard icon={Satellite} label="Định vị GPS" value="8 vệ tinh" sub="Đã khóa vị trí" tone="ok" />
        <StatCard icon={Database} label="Thẻ nhớ SD" value="4.2 / 16 GB" sub={flightState === "flying" ? "Đang ghi log" : "Sẵn sàng ghi"} tone="ok" />
        <StatCard icon={Battery} label="Mức pin" value="82%" sub="Còn khoảng 5h 40p" tone="neutral" />
      </div>

      <div className="grid grid-cols-3 gap-4 mt-6">
        <Card style={{ gridColumn: "span 2" }}>
          <div className="flex items-center justify-between mb-4">
            <div className="flex items-center gap-2">
              <Volume2 size={16} style={{ color: C.teal }} />
              <span className="text-sm font-semibold" style={{ color: C.ink }}>Kiểm tra còi báo động</span>
            </div>
            <span className="text-xs px-2 py-1 rounded-full" style={{ background: C.navySoft, color: C.navy, fontFamily: FONT_MONO }}>GP2</span>
          </div>
          <div className="grid grid-cols-4 gap-2 mb-4">
            {[
              { label: "Bíp ngắn", cmd: "Đang kêu 1 tiếng bíp..." },
              { label: "Boot OK", cmd: "Boot OK — 2 tiếng bíp" },
              { label: "Boot Error", cmd: "Boot Error — 1 tiếng dài" },
              { label: "Định vị", cmd: "Chế độ định vị — lặp 3s" },
            ].map((b) => (
              <button
                key={b.label}
                onClick={() => runBuzzerTest(b.cmd)}
                className="text-xs font-medium py-2 rounded-lg"
                style={{ border: `1px solid ${C.border}`, color: C.inkSoft, background: C.surface }}
              >
                {b.label}
              </button>
            ))}
          </div>
          <div className="flex items-center gap-2 px-3 py-2 rounded-lg" style={{ background: C.bg }}>
            <RefreshCw size={12} style={{ color: C.teal }} />
            <span className="text-xs" style={{ color: C.inkSoft, fontFamily: FONT_MONO }}>{buzzerStatus}</span>
          </div>
        </Card>

        <Card>
          <div className="flex items-center gap-2 mb-4">
            <AlertTriangle size={16} style={{ color: C.teal }} />
            <span className="text-sm font-semibold" style={{ color: C.ink }}>Cảnh báo gần đây</span>
          </div>
          <div className="space-y-3">
            {MOCK.alerts.map((a, i) => (
              <div key={i} className="flex items-start gap-2">
                <div className="mt-1 rounded-full" style={{ width: 6, height: 6, background: a.level === "ok" ? C.green : C.amber }} />
                <div>
                  <div className="text-xs" style={{ color: C.ink }}>{a.msg}</div>
                  <div className="text-[11px]" style={{ color: C.inkFaint, fontFamily: FONT_MONO }}>{a.time}</div>
                </div>
              </div>
            ))}
          </div>
        </Card>
      </div>
    </div>
  );
}

function GPSSection() {
  const g = MOCK.gps;
  return (
    <div>
      <SectionTitle eyebrow="GPS & Vệ tinh" title="Định vị và thời gian vệ tinh" />
      <div className="grid grid-cols-3 gap-4">
        <Card style={{ gridColumn: "span 2" }}>
          <div className="text-xs font-medium mb-4" style={{ color: C.inkSoft }}>Tọa độ hiện tại</div>
          <div className="grid grid-cols-2 gap-6">
            <div>
              <div className="text-[11px] mb-1" style={{ color: C.inkFaint }}>VĨ ĐỘ (LAT)</div>
              <div className="text-3xl font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{g.lat}°N</div>
            </div>
            <div>
              <div className="text-[11px] mb-1" style={{ color: C.inkFaint }}>KINH ĐỘ (LON)</div>
              <div className="text-3xl font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{g.lon}°E</div>
            </div>
          </div>
          <div className="mt-6 relative rounded-xl overflow-hidden" style={{ height: 180, background: C.tealSoft, backgroundImage: `linear-gradient(${C.border} 1px, transparent 1px), linear-gradient(90deg, ${C.border} 1px, transparent 1px)`, backgroundSize: "24px 24px" }}>
            <div className="absolute" style={{ top: "48%", left: "52%", transform: "translate(-50%,-50%)" }}>
              <div className="rounded-full" style={{ width: 14, height: 14, background: C.teal, border: "3px solid #fff", boxShadow: `0 0 0 3px ${C.teal}33` }} />
            </div>
            <span className="absolute bottom-2 right-3 text-[10px]" style={{ color: C.inkFaint, fontFamily: FONT_MONO }}>bản đồ minh họa — demo</span>
          </div>
        </Card>
        <div className="space-y-4">
          <Card>
            <div className="flex items-center gap-2 mb-2"><Satellite size={14} style={{ color: C.teal }} /><span className="text-xs" style={{ color: C.inkSoft }}>Vệ tinh sử dụng</span></div>
            <div className="text-2xl font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{g.sats}</div>
          </Card>
          <Card>
            <div className="flex items-center gap-2 mb-2"><Navigation size={14} style={{ color: C.teal, transform: `rotate(${g.heading}deg)` }} /><span className="text-xs" style={{ color: C.inkSoft }}>Tốc độ / Hướng</span></div>
            <div className="text-2xl font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{g.speed} km/h</div>
            <div className="text-[11px] mt-1" style={{ color: C.inkFaint }}>{g.heading}° — Nam</div>
          </Card>
          <Card>
            <div className="flex items-center gap-2 mb-2"><Clock size={14} style={{ color: C.teal }} /><span className="text-xs" style={{ color: C.inkSoft }}>UTC (đồng bộ GPS)</span></div>
            <div className="text-base font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{g.utc}</div>
          </Card>
        </div>
      </div>
    </div>
  );
}

// ---- 3D attitude indicator (Three.js) ----
function Attitude3D({ pitch, roll, heading }) {
  const mountRef = useRef(null);

  useEffect(() => {
    const width = 280, height = 230;
    const scene = new THREE.Scene();

    const camera = new THREE.PerspectiveCamera(36, width / height, 0.1, 100);
    camera.position.set(2.6, 1.7, 4.6);
    camera.lookAt(0, 0.1, 0);

    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    renderer.setSize(width, height);
    renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
    if (mountRef.current) {
      mountRef.current.innerHTML = "";
      mountRef.current.appendChild(renderer.domElement);
    }

    scene.add(new THREE.AmbientLight(0xffffff, 0.75));
    const dir = new THREE.DirectionalLight(0xffffff, 0.9);
    dir.position.set(3, 5, 4);
    scene.add(dir);
    const dir2 = new THREE.DirectionalLight(0x0e7c86, 0.25);
    dir2.position.set(-4, 2, -3);
    scene.add(dir2);

    // ground / horizon reference
    const groundGeo = new THREE.CircleGeometry(3.6, 48);
    const groundMat = new THREE.MeshStandardMaterial({ color: 0x0e7c86, transparent: true, opacity: 0.12, side: THREE.DoubleSide });
    const ground = new THREE.Mesh(groundGeo, groundMat);
    ground.rotation.x = -Math.PI / 2;
    ground.position.y = -0.95;
    scene.add(ground);

    const grid = new THREE.GridHelper(7, 14, 0x0e7c86, 0xcbd1da);
    grid.position.y = -0.95;
    grid.material.transparent = true;
    grid.material.opacity = 0.4;
    scene.add(grid);

    // aircraft group
    const plane = new THREE.Group();
    const bodyMat = new THREE.MeshStandardMaterial({ color: 0xebf0f5, metalness: 0.15, roughness: 0.55 });
    const accentMat = new THREE.MeshStandardMaterial({ color: 0x0e7c86, metalness: 0.2, roughness: 0.4 });
    const darkMat = new THREE.MeshStandardMaterial({ color: 0x1b3a5c, metalness: 0.2, roughness: 0.45 });

    const fuselage = new THREE.Mesh(new THREE.CylinderGeometry(0.14, 0.05, 2.1, 16), bodyMat);
    fuselage.rotation.x = Math.PI / 2;
    plane.add(fuselage);

    const nose = new THREE.Mesh(new THREE.ConeGeometry(0.14, 0.36, 16), accentMat);
    nose.rotation.x = Math.PI / 2;
    nose.position.z = -1.23;
    plane.add(nose);

    const wings = new THREE.Mesh(new THREE.BoxGeometry(2.5, 0.05, 0.46), darkMat);
    wings.position.set(0, -0.02, 0.1);
    plane.add(wings);

    const tailH = new THREE.Mesh(new THREE.BoxGeometry(0.95, 0.04, 0.28), darkMat);
    tailH.position.set(0, -0.02, 0.95);
    plane.add(tailH);

    const tailV = new THREE.Mesh(new THREE.BoxGeometry(0.04, 0.46, 0.32), accentMat);
    tailV.position.set(0, 0.24, 0.95);
    plane.add(tailV);

    scene.add(plane);

    let raf;
    let t = 0;
    const animate = () => {
      t += 0.012;
      const bob = Math.sin(t) * 0.015;
      plane.rotation.z = -THREE.MathUtils.degToRad(roll);
      plane.rotation.x = THREE.MathUtils.degToRad(pitch) + bob;
      plane.rotation.y = THREE.MathUtils.degToRad(heading);
      renderer.render(scene, camera);
      raf = requestAnimationFrame(animate);
    };
    animate();

    return () => {
      cancelAnimationFrame(raf);
      renderer.dispose();
      groundGeo.dispose();
      groundMat.dispose();
      if (mountRef.current) mountRef.current.innerHTML = "";
    };
  }, [pitch, roll, heading]);

  return <div ref={mountRef} style={{ width: 280, height: 230 }} />;
}

function attitudeStatus(pitch, roll) {
  const p = Math.abs(pitch), r = Math.abs(roll);
  if (p > 35 || r > 45) return { tone: "bad", label: "Vượt ngưỡng an toàn" };
  if (p > 20 || r > 25) return { tone: "warn", label: "Gần ngưỡng cảnh báo" };
  return { tone: "ok", label: "Trong ngưỡng bình thường" };
}

function ReadoutRow({ icon: Icon, label, value, unit, hint, tone = "neutral" }) {
  const tones = { ok: C.green, warn: C.amber, bad: C.red, neutral: C.ink };
  return (
    <div className="flex items-center justify-between py-3" style={{ borderBottom: `1px solid ${C.border}` }}>
      <div className="flex items-center gap-3">
        <div className="flex items-center justify-center rounded-lg" style={{ width: 30, height: 30, background: C.tealSoft }}>
          <Icon size={14} style={{ color: C.teal }} />
        </div>
        <div>
          <div className="text-xs" style={{ color: C.inkSoft }}>{label}</div>
          {hint && <div className="text-[11px]" style={{ color: C.inkFaint }}>{hint}</div>}
        </div>
      </div>
      <div className="text-lg font-semibold" style={{ color: tones[tone], fontFamily: FONT_MONO }}>
        {value}<span className="text-xs ml-1" style={{ color: C.inkFaint }}>{unit}</span>
      </div>
    </div>
  );
}

function TiltBar({ label, value, min = -45, max = 45, tone = "neutral" }) {
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
          className="absolute top-0 bottom-0 rounded-full"
          style={{
            left: value >= 0 ? "50%" : `${pct}%`,
            width: `${Math.abs(pct - 50)}%`,
            background: tones[tone],
          }}
        />
      </div>
    </div>
  );
}

function MotionSection() {
  const m = MOCK.motion;
  const status = attitudeStatus(m.pitch, m.roll);
  const pitchTone = Math.abs(m.pitch) > 35 ? "bad" : Math.abs(m.pitch) > 20 ? "warn" : "ok";
  const rollTone = Math.abs(m.roll) > 45 ? "bad" : Math.abs(m.roll) > 25 ? "warn" : "ok";

  return (
    <div>
      <SectionTitle eyebrow="Chuyển động" title="Cảm biến quán tính (BMI160)" />

      <div className="grid grid-cols-3 gap-4">
        <Card style={{ gridColumn: "span 2" }}>
          <div className="flex items-center justify-between mb-1">
            <div className="flex items-center gap-2">
              <Plane size={16} style={{ color: C.teal }} />
              <span className="text-sm font-semibold" style={{ color: C.ink }}>Tư thế máy bay (mô phỏng 3D)</span>
            </div>
            <StatusPill icon={status.tone === "ok" ? CheckCircle2 : AlertTriangle} label={status.label} tone={status.tone} />
          </div>
          <div className="text-[11px] mb-3" style={{ color: C.inkFaint }}>
            Mô hình xoay theo dữ liệu pitch, roll và heading thời gian thực từ IMU
          </div>
          <div className="flex items-center justify-center rounded-xl" style={{ background: `linear-gradient(180deg, ${C.tealSoft} 0%, ${C.bg} 100%)`, border: `1px solid ${C.border}` }}>
            <Attitude3D pitch={m.pitch} roll={m.roll} heading={m.heading} />
          </div>
          <div className="flex items-center justify-center gap-6 mt-2">
            <div className="flex items-center gap-1.5">
              <div className="rounded-full" style={{ width: 8, height: 8, background: C.accent || C.teal }} />
              <span className="text-[11px]" style={{ color: C.inkFaint }}>Mũi máy bay</span>
            </div>
            <div className="flex items-center gap-1.5">
              <div className="rounded-full" style={{ width: 8, height: 8, background: C.navy }} />
              <span className="text-[11px]" style={{ color: C.inkFaint }}>Cánh & đuôi</span>
            </div>
          </div>
        </Card>

        <div className="space-y-4">
          <Card>
            <div className="text-xs mb-3" style={{ color: C.inkSoft }}>Chỉ số tư thế tức thời</div>
            <ReadoutRow icon={Gauge} label="Lực G tổng hợp" hint="Ngưỡng bình thường: 0.8 – 1.3 G" value={m.gforce} unit="G" tone="ok" />
            <ReadoutRow icon={Plane} label="Góc chúc/ngẩng (Pitch)" hint={m.pitch >= 0 ? "Mũi máy bay hướng lên" : "Mũi máy bay hướng xuống"} value={(m.pitch > 0 ? "+" : "") + m.pitch} unit="°" tone={pitchTone} />
            <ReadoutRow icon={Compass} label="Góc nghiêng cánh (Roll)" hint={m.roll >= 0 ? "Nghiêng phải" : "Nghiêng trái"} value={(m.roll > 0 ? "+" : "") + m.roll} unit="°" tone={rollTone} />
            <div style={{ paddingTop: 12 }}>
              <ReadoutRow icon={Navigation} label="Hướng bay (Heading)" hint="La bàn — 0° = Bắc" value={m.heading} unit="°" tone="neutral" />
            </div>
          </Card>
        </div>
      </div>

      <Card style={{ marginTop: 16 }}>
        <div className="text-xs mb-4" style={{ color: C.inkSoft }}>Góc nghiêng chi tiết</div>
        <div className="grid grid-cols-2 gap-8">
          <TiltBar label="Pitch (nghiêng dọc)" value={m.pitch} tone={pitchTone} />
          <TiltBar label="Roll (nghiêng ngang)" value={m.roll} tone={rollTone} />
        </div>
      </Card>

      <div className="grid grid-cols-2 gap-4 mt-4">
        <Card>
          <div className="text-xs mb-1" style={{ color: C.inkSoft }}>Gia tốc kế (Accelerometer)</div>
          <div className="text-[11px] mb-3" style={{ color: C.inkFaint }}>Đơn vị: g (1g ≈ 9.8 m/s²) — trục X/Y/Z thân máy bay</div>
          <ResponsiveContainer width="100%" height={140}>
            <BarChart data={m.accel} layout="vertical" margin={{ left: 8, right: 16 }}>
              <CartesianGrid horizontal={false} stroke={C.border} />
              <XAxis type="number" domain={[-1, 1]} tick={{ fontSize: 10, fill: C.inkFaint }} />
              <YAxis type="category" dataKey="axis" tick={{ fontSize: 11, fill: C.inkSoft }} width={20} />
              <Tooltip contentStyle={{ fontSize: 11, borderRadius: 8, border: `1px solid ${C.border}` }} />
              <Bar dataKey="g" fill={C.teal} radius={[0, 4, 4, 0]} barSize={16} />
            </BarChart>
          </ResponsiveContainer>
        </Card>
        <Card>
          <div className="text-xs mb-1" style={{ color: C.inkSoft }}>Con quay hồi chuyển (Gyroscope)</div>
          <div className="text-[11px] mb-3" style={{ color: C.inkFaint }}>Đơn vị: độ/giây (°/s) — tốc độ xoay quanh từng trục</div>
          <ResponsiveContainer width="100%" height={140}>
            <BarChart data={m.gyro} layout="vertical" margin={{ left: 8, right: 16 }}>
              <CartesianGrid horizontal={false} stroke={C.border} />
              <XAxis type="number" domain={[-20, 20]} tick={{ fontSize: 10, fill: C.inkFaint }} />
              <YAxis type="category" dataKey="axis" tick={{ fontSize: 11, fill: C.inkSoft }} width={20} />
              <Tooltip contentStyle={{ fontSize: 11, borderRadius: 8, border: `1px solid ${C.border}` }} />
              <Bar dataKey="dps" fill={C.navy} radius={[0, 4, 4, 0]} barSize={16} />
            </BarChart>
          </ResponsiveContainer>
        </Card>
      </div>
    </div>
  );
}

function StorageSection() {
  const s = MOCK.storage;
  const pct = Math.round((s.usedGB / s.totalGB) * 100);
  return (
    <div>
      <SectionTitle eyebrow="Lưu trữ" title="Hệ thống lưu trữ SD Card" />
      <div className="grid grid-cols-3 gap-4">
        <Card style={{ gridColumn: "span 2" }}>
          <div className="flex items-center justify-between mb-3">
            <span className="text-xs" style={{ color: C.inkSoft }}>Dung lượng đã sử dụng</span>
            <span className="text-xs font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{s.usedGB} / {s.totalGB} GB</span>
          </div>
          <div className="rounded-full overflow-hidden" style={{ height: 10, background: C.bg, border: `1px solid ${C.border}` }}>
            <div style={{ width: `${pct}%`, height: "100%", background: C.teal }} />
          </div>
          <div className="flex items-center gap-2 mt-4">
            <div className="rounded-full" style={{ width: 8, height: 8, background: C.green }} />
            <span className="text-xs" style={{ color: C.ink }}>Đang ghi: {s.filename}</span>
            <span className="text-[11px] ml-auto" style={{ color: C.inkFaint, fontFamily: FONT_MONO }}>{s.writeRate}</span>
          </div>
        </Card>
        <Card>
          <div className="text-xs mb-2" style={{ color: C.inkSoft }}>Hệ thống tệp</div>
          <div className="text-2xl font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>FAT32</div>
          <div className="text-[11px] mt-1" style={{ color: C.inkFaint }}>Thẻ nhớ: đã nhận diện</div>
        </Card>
      </div>
      <Card style={{ marginTop: 16 }}>
        <div className="text-xs mb-3" style={{ color: C.inkSoft }}>Tệp log gần đây</div>
        <div className="divide-y" style={{ borderColor: C.border }}>
          {s.files.map((f, i) => (
            <div key={i} className="flex items-center justify-between py-2.5">
              <div className="flex items-center gap-2">
                <FileText size={14} style={{ color: C.teal }} />
                <span className="text-xs" style={{ color: C.ink, fontFamily: FONT_MONO }}>{f.name}</span>
              </div>
              <div className="flex items-center gap-6">
                <span className="text-xs" style={{ color: C.inkFaint }}>{f.date}</span>
                <span className="text-xs" style={{ color: C.inkSoft, fontFamily: FONT_MONO }}>{f.size}</span>
              </div>
            </div>
          ))}
        </div>
      </Card>
    </div>
  );
}

function SystemSection() {
  const s = MOCK.system;
  return (
    <div>
      <SectionTitle eyebrow="Hệ thống" title="Thông tin thiết bị & kết nối" />
      <div className="grid grid-cols-3 gap-4">
        <Card>
          <div className="text-xs mb-2" style={{ color: C.inkSoft }}>Bo mạch</div>
          <div className="text-base font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{s.board}</div>
          <div className="text-[11px] mt-1" style={{ color: C.inkFaint }}>Firmware {s.firmware}</div>
        </Card>
        <Card>
          <div className="text-xs mb-2" style={{ color: C.inkSoft }}>Thời gian hoạt động</div>
          <div className="text-base font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{s.uptime}</div>
        </Card>
        <Card>
          <div className="flex items-center justify-between">
            <div>
              <div className="text-xs mb-2" style={{ color: C.inkSoft }}>Kết nối MQTT</div>
              <div className="text-base font-semibold" style={{ color: C.ink, fontFamily: FONT_MONO }}>{s.mqtt ? "Đã kết nối" : "Mất kết nối"}</div>
            </div>
            {s.mqtt ? <Wifi size={18} style={{ color: C.green }} /> : <WifiOff size={18} style={{ color: C.red }} />}
          </div>
          <div className="text-[11px] mt-2" style={{ color: C.inkFaint, fontFamily: FONT_MONO }}>{s.broker}</div>
        </Card>
      </div>

      <Card style={{ marginTop: 16 }}>
        <div className="text-xs mb-3" style={{ color: C.inkSoft }}>Bản đồ chân kết nối ngoại vi</div>
        <table className="w-full" style={{ borderCollapse: "collapse" }}>
          <thead>
            <tr>
              {["Bus", "Thiết bị", "Chân GPIO"].map((h) => (
                <th key={h} className="text-left text-[11px] font-medium pb-2" style={{ color: C.inkFaint, borderBottom: `1px solid ${C.border}` }}>{h}</th>
              ))}
            </tr>
          </thead>
          <tbody>
            {s.pins.map((p, i) => (
              <tr key={i}>
                <td className="text-xs py-2" style={{ color: C.navy, fontFamily: FONT_MONO, borderBottom: `1px solid ${C.border}` }}>{p.bus}</td>
                <td className="text-xs py-2" style={{ color: C.ink, borderBottom: `1px solid ${C.border}` }}>{p.dev}</td>
                <td className="text-xs py-2" style={{ color: C.inkSoft, fontFamily: FONT_MONO, borderBottom: `1px solid ${C.border}` }}>{p.pins}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </Card>
    </div>
  );
}

// ---- Flight replay helpers ----
function parseDurationToMinutes(str) {
  const hMatch = str.match(/(\d+)\s*h/);
  const mMatch = str.match(/(\d+)\s*p/);
  const h = hMatch ? parseInt(hMatch[1], 10) : 0;
  const m = mMatch ? parseInt(mMatch[1], 10) : 0;
  return h * 60 + (m || (h === 0 ? parseInt(str, 10) || 0 : 0));
}

function formatTime(sec) {
  const s = Math.max(0, Math.round(sec));
  const mm = Math.floor(s / 60);
  const ss = s % 60;
  return `${String(mm).padStart(2, "0")}:${String(ss).padStart(2, "0")}`;
}

function generateTrack(flight) {
  const N = 36;
  const maxSpeedNum = parseInt(flight.maxSpeed, 10) || 70;
  const durationMin = parseDurationToMinutes(flight.duration) || 20;
  const totalSec = durationMin * 60;
  const alertIndex = flight.status === "alert" ? Math.round(N * 0.62) : -1;

  let x = 30, y = 120, heading = 60;
  const track = [];
  for (let i = 0; i < N; i++) {
    const progress = i / (N - 1);
    const throttle = 0.25 + 0.75 * Math.sin(progress * Math.PI);
    const wobble = i === alertIndex ? 0.55 : 0.12 * Math.sin(i * 1.6);
    const speed = Math.max(0, Math.round(maxSpeedNum * throttle * (1 - wobble) + (i === alertIndex ? -maxSpeedNum * 0.3 : 0)));
    const altitude = Math.round(70 + 55 * Math.sin(progress * Math.PI * 1.15 + 0.4) - (i === alertIndex ? 22 : 0));
    heading = (heading + Math.sin(i * 0.55) * 22 + 360) % 360;
    x += Math.cos((heading * Math.PI) / 180) * 6.2;
    y += Math.sin((heading * Math.PI) / 180) * 3.4;
    x = Math.max(14, Math.min(246, x));
    y = Math.max(14, Math.min(146, y));
    const gforce = +(1 + Math.sin(i * 0.8) * 0.12 + (i === alertIndex ? 0.45 : 0)).toFixed(2);
    track.push({
      i, t: Math.round(progress * totalSec), speed, altitude,
      heading: Math.round(heading), gforce,
      x: +x.toFixed(1), y: +y.toFixed(1), alert: i === alertIndex,
    });
  }
  return { track, alertIndex, totalSec };
}

function FlightReplayModal({ flight, onClose }) {
  const { track, alertIndex, totalSec } = useMemo(() => generateTrack(flight), [flight]);
  const [index, setIndex] = useState(0);
  const [playing, setPlaying] = useState(false);
  const point = track[index];

  useEffect(() => {
    if (!playing) return;
    const id = setInterval(() => {
      setIndex((i) => {
        if (i >= track.length - 1) {
          setPlaying(false);
          return i;
        }
        return i + 1;
      });
    }, 220);
    return () => clearInterval(id);
  }, [playing, track.length]);

  const pathD = track.map((p, i) => `${i === 0 ? "M" : "L"} ${p.x} ${p.y}`).join(" ");
  const traveledD = track.slice(0, index + 1).map((p, i) => `${i === 0 ? "M" : "L"} ${p.x} ${p.y}`).join(" ");
  const alertPassed = alertIndex >= 0 && index >= alertIndex;

  return (
    <div
      className="fixed inset-0 flex items-center justify-center z-50"
      style={{ background: "rgba(20,24,31,0.55)" }}
      onClick={onClose}
    >
      <div
        onClick={(e) => e.stopPropagation()}
        style={{ background: C.surface, borderRadius: 16, width: 780, maxWidth: "92vw", maxHeight: "90vh", overflowY: "auto", border: `1px solid ${C.border}` }}
      >
        <div className="flex items-center justify-between px-6 py-4" style={{ borderBottom: `1px solid ${C.border}` }}>
          <div>
            <div className="text-xs" style={{ color: C.teal, fontWeight: 600 }}>XEM LẠI CHUYẾN BAY</div>
            <div className="text-lg font-semibold" style={{ color: C.ink }}>{flight.date}</div>
          </div>
          <div className="flex items-center gap-3">
            <StatusPill icon={flight.status === "normal" ? CheckCircle2 : AlertTriangle} label={flight.status === "normal" ? "Bình thường" : "Có cảnh báo"} tone={flight.status === "normal" ? "ok" : "warn"} />
            <button onClick={onClose} className="flex items-center justify-center rounded-lg" style={{ width: 30, height: 30, background: C.bg }}>
              <X size={15} style={{ color: C.inkSoft }} />
            </button>
          </div>
        </div>

        <div className="px-6 py-5">
          <div className="grid grid-cols-3 gap-4">
            <Card style={{ gridColumn: "span 2", padding: 16 }}>
              <div className="flex items-center justify-between mb-2">
                <div className="flex items-center gap-2">
                  <MapPinned size={14} style={{ color: C.teal }} />
                  <span className="text-xs font-semibold" style={{ color: C.ink }}>Đường bay đã ghi</span>
                </div>
                {alertPassed && <StatusPill icon={AlertTriangle} label="Cảnh báo tại " value={formatTime(track[alertIndex].t)} tone="warn" />}
              </div>
              <svg viewBox="0 0 260 160" style={{ width: "100%", height: 200, background: C.tealSoft, borderRadius: 10 }}>
                <defs>
                  <pattern id="grid" width="20" height="20" patternUnits="userSpaceOnUse">
                    <path d="M 20 0 L 0 0 0 20" fill="none" stroke={C.border} strokeWidth="1" />
                  </pattern>
                </defs>
                <rect width="260" height="160" fill="url(#grid)" />
                <path d={pathD} fill="none" stroke={C.borderStrong} strokeWidth="2" strokeDasharray="3 3" />
                <path d={traveledD} fill="none" stroke={C.teal} strokeWidth="3" />
                {alertIndex >= 0 && (
                  <circle cx={track[alertIndex].x} cy={track[alertIndex].y} r="6" fill={C.redSoft} stroke={C.red} strokeWidth="2" />
                )}
                <circle cx={track[0].x} cy={track[0].y} r="4" fill={C.navy} />
                <g transform={`translate(${point.x} ${point.y}) rotate(${point.heading + 90})`}>
                  <circle r="9" fill={C.surface} stroke={C.teal} strokeWidth="2" />
                  <path d="M 0 -5 L 4 4 L 0 2 L -4 4 Z" fill={C.teal} />
                </g>
              </svg>
              <div className="flex items-center justify-between mt-3 text-[11px]" style={{ color: C.inkFaint }}>
                <span>● Điểm cất cánh</span>
                <span>— — quãng đường còn lại</span>
                <span style={{ color: C.teal }}>— đã bay qua</span>
              </div>
            </Card>

            <Card style={{ padding: 16 }}>
              <div className="text-xs font-semibold mb-3" style={{ color: C.ink }}>Chỉ số tại thời điểm này</div>
              <ReadoutRow icon={Gauge} label="Tốc độ" value={point.speed} unit="km/h" tone="ok" />
              <ReadoutRow icon={Activity} label="Độ cao" value={point.altitude} unit="m" tone="ok" />
              <ReadoutRow icon={Navigation} label="Hướng bay" value={point.heading} unit="°" tone="neutral" />
              <ReadoutRow icon={Plane} label="Lực G" value={point.gforce} unit="G" tone={point.gforce > 1.3 ? "bad" : "ok"} />
            </Card>
          </div>

          <div className="grid grid-cols-2 gap-4 mt-4">
            <Card style={{ padding: 16 }}>
              <div className="text-[11px] mb-2" style={{ color: C.inkSoft }}>Tốc độ theo thời gian (km/h)</div>
              <ResponsiveContainer width="100%" height={110}>
                <LineChart data={track} margin={{ left: -20, right: 8, top: 4 }}>
                  <XAxis dataKey="t" hide />
                  <YAxis hide />
                  <Line type="monotone" dataKey="speed" stroke={C.teal} strokeWidth={2} dot={false} />
                  <ReferenceDot x={point.t} y={point.speed} r={4} fill={C.teal} stroke="#fff" />
                </LineChart>
              </ResponsiveContainer>
            </Card>
            <Card style={{ padding: 16 }}>
              <div className="text-[11px] mb-2" style={{ color: C.inkSoft }}>Độ cao theo thời gian (m)</div>
              <ResponsiveContainer width="100%" height={110}>
                <LineChart data={track} margin={{ left: -20, right: 8, top: 4 }}>
                  <XAxis dataKey="t" hide />
                  <YAxis hide />
                  <Line type="monotone" dataKey="altitude" stroke={C.navy} strokeWidth={2} dot={false} />
                  <ReferenceDot x={point.t} y={point.altitude} r={4} fill={C.navy} stroke="#fff" />
                </LineChart>
              </ResponsiveContainer>
            </Card>
          </div>

          <Card style={{ marginTop: 16, padding: 16 }}>
            <div className="flex items-center gap-4">
              <button
                onClick={() => setPlaying((p) => !p)}
                className="flex items-center justify-center rounded-full flex-shrink-0"
                style={{ width: 40, height: 40, background: C.navy }}
              >
                {playing ? <Pause size={16} color="#fff" /> : <Play size={16} color="#fff" style={{ marginLeft: 2 }} />}
              </button>
              <div className="flex-1">
                <input
                  type="range"
                  min={0}
                  max={track.length - 1}
                  value={index}
                  onChange={(e) => { setPlaying(false); setIndex(parseInt(e.target.value, 10)); }}
                  className="w-full"
                  style={{ accentColor: C.teal }}
                />
                <div className="flex items-center justify-between mt-1 text-[11px]" style={{ color: C.inkFaint, fontFamily: FONT_MONO }}>
                  <span>{formatTime(point.t)}</span>
                  <span>{flight.duration} · {flight.distance}</span>
                  <span>{formatTime(totalSec)}</span>
                </div>
              </div>
            </div>
          </Card>
        </div>
      </div>
    </div>
  );
}

function HistorySection() {
  const [selected, setSelected] = useState(null);
  return (
    <div>
      <SectionTitle eyebrow="Lịch sử" title="Nhật ký chuyến bay" />
      <Card>
        <div className="divide-y" style={{ borderColor: C.border }}>
          {MOCK.history.map((h, i) => (
            <div key={i} className="flex items-center justify-between py-3">
              <div className="flex items-center gap-3">
                <div className="rounded-full" style={{ width: 8, height: 8, background: h.status === "normal" ? C.green : C.amber }} />
                <div>
                  <div className="text-sm" style={{ color: C.ink }}>{h.date}</div>
                  <div className="text-[11px]" style={{ color: C.inkFaint }}>{h.status === "normal" ? "Bình thường" : "Có cảnh báo"}</div>
                </div>
              </div>
              <div className="flex items-center gap-6 text-xs" style={{ color: C.inkSoft, fontFamily: FONT_MONO }}>
                <span>{h.duration}</span>
                <span>{h.distance}</span>
                <span>{h.maxSpeed}</span>
              </div>
              <button
                onClick={() => setSelected(i)}
                className="flex items-center gap-1 text-xs font-medium px-3 py-1.5 rounded-lg"
                style={{ color: C.teal, border: `1px solid ${C.border}` }}
              >
                <PlayCircle size={13} /> Xem lại
              </button>
            </div>
          ))}
        </div>
      </Card>
      {selected !== null && (
        <FlightReplayModal flight={MOCK.history[selected]} onClose={() => setSelected(null)} />
      )}
    </div>
  );
}

// ---- Flight control panel ----
const TEST_ITEMS = [
  { key: "gps", label: "Định vị GPS", desc: "Kiểm tra khóa vệ tinh Neo-6M" },
  { key: "imu", label: "Cảm biến IMU", desc: "Kiểm tra BMI160 (I2C0)" },
  { key: "sd", label: "Thẻ nhớ SD", desc: "Kiểm tra ghi/đọc thẻ nhớ" },
  { key: "sim", label: "SIM & MQTT", desc: "Kiểm tra kết nối A7680C" },
  { key: "buzzer", label: "Còi báo", desc: "Kiểm tra còi cảnh báo (GP2)" },
];

const ALERT_TYPES = [
  { type: "emergency", icon: ShieldAlert, label: "Cảnh báo khẩn cấp", tone: "bad" },
  { type: "support", icon: Send, label: "Yêu cầu hỗ trợ mặt đất", tone: "warn" },
  { type: "position", icon: MapPin, label: "Báo cáo vị trí hiện tại", tone: "neutral" },
  { type: "ping", icon: Radio, label: "Kiểm tra tín hiệu", tone: "neutral" },
];

function StepHeader({ number, title, done }) {
  return (
    <div className="flex items-center gap-3 mb-4">
      <div
        className="flex items-center justify-center rounded-full flex-shrink-0"
        style={{ width: 26, height: 26, background: done ? C.green : C.navy, color: "#fff", fontFamily: FONT_MONO, fontSize: 12, fontWeight: 600 }}
      >
        {done ? <CheckCircle2 size={14} /> : number}
      </div>
      <span className="text-sm font-semibold" style={{ color: C.ink }}>{title}</span>
    </div>
  );
}

function TestItemRow({ item, status, onRun }) {
  const icons = { idle: Circle, testing: Loader2, pass: CheckCircle2, fail: XCircle };
  const colors = { idle: C.inkFaint, testing: C.teal, pass: C.green, fail: C.red };
  const labels = { idle: "Chưa kiểm tra", testing: "Đang kiểm tra…", pass: "Đạt", fail: "Lỗi" };
  const Icon = icons[status];
  return (
    <div className="flex items-center justify-between py-2.5" style={{ borderBottom: `1px solid ${C.border}` }}>
      <div>
        <div className="text-xs font-medium" style={{ color: C.ink }}>{item.label}</div>
        <div className="text-[11px]" style={{ color: C.inkFaint }}>{item.desc}</div>
      </div>
      <div className="flex items-center gap-3">
        <div className="flex items-center gap-1.5">
          <Icon size={13} style={{ color: colors[status] }} className={status === "testing" ? "animate-spin" : ""} />
          <span className="text-[11px]" style={{ color: colors[status], fontFamily: FONT_MONO }}>{labels[status]}</span>
        </div>
        <button
          onClick={onRun}
          disabled={status === "testing"}
          className="text-[11px] font-medium px-2.5 py-1 rounded-md"
          style={{ border: `1px solid ${C.border}`, color: C.inkSoft, background: C.surface }}
        >
          Kiểm tra
        </button>
      </div>
    </div>
  );
}

function ControlSection({ flightState, setFlightState }) {
  const [tests, setTests] = useState({ gps: "idle", imu: "idle", sd: "idle", sim: "idle", buzzer: "idle" });
  const [setup, setSetup] = useState({
    filename: "log_20260702_flight.csv",
    pilot: "",
    aircraftId: "YD-RP2040-01",
    notes: "",
  });
  const [alertLog, setAlertLog] = useState([]);
  const [autoTrigger, setAutoTrigger] = useState(false);

  const runTest = (key) => {
    setTests((t) => ({ ...t, [key]: "testing" }));
    setTimeout(() => {
      setTests((t) => ({ ...t, [key]: "pass" }));
    }, 900 + Math.random() * 500);
  };
  const runAllTests = () => TEST_ITEMS.forEach((it) => runTest(it.key));

  const allPass = Object.values(tests).every((v) => v === "pass");
  const testsDone = allPass;
  const setupDone = setup.filename.trim() !== "" && setup.pilot.trim() !== "";
  const canStart = testsDone && setupDone && flightState !== "flying";

  const nowStr = () => new Date().toLocaleTimeString("vi-VN", { hour: "2-digit", minute: "2-digit" });

  const startFlight = () => {
    if (!canStart) return;
    setFlightState("flying");
    setAlertLog((log) => [{ time: nowStr(), msg: `Bắt đầu ghi log: ${setup.filename}`, level: "ok" }, ...log]);
  };
  const endFlight = () => {
    setFlightState("ended");
    setAlertLog((log) => [{ time: nowStr(), msg: "Đã kết thúc chuyến bay — ngừng ghi log", level: "ok" }, ...log]);
  };
  const newFlight = () => {
    setFlightState("idle");
    setTests({ gps: "idle", imu: "idle", sd: "idle", sim: "idle", buzzer: "idle" });
    setAlertLog([]);
  };
  const sendAlert = (item) => {
    setAlertLog((log) => [{ time: nowStr(), msg: `Đã gửi: ${item.label}`, level: item.tone === "bad" ? "warn" : "ok" }, ...log]);
  };

  const flightLabel = {
    idle: "Chưa sẵn sàng", ready: "Sẵn sàng cất cánh", flying: "Đang bay — đang ghi log", ended: "Đã kết thúc chuyến bay",
  }[flightState];
  const flightTone = flightState === "flying" ? "ok" : flightState === "ended" ? "warn" : "neutral";

  return (
    <div>
      <SectionTitle eyebrow="Điều khiển" title="Điều khiển chuyến bay" />

      <Card style={{ marginBottom: 16, background: C.navySoft, border: `1px solid ${C.navySoft}` }}>
        <div className="flex items-center gap-2 mb-2">
          <Settings2 size={15} style={{ color: C.navy }} />
          <span className="text-sm font-semibold" style={{ color: C.navy }}>Hộp đen nhận lệnh bắt đầu bằng cách nào?</span>
        </div>
        <p className="text-xs mb-3" style={{ color: C.inkSoft }}>
          Thiết bị chỉ bắt đầu ghi log khi nhận được một trong các tín hiệu sau — bảng điều khiển này thực hiện đường tín hiệu điều khiển từ xa (1):
        </p>
        <div className="flex flex-wrap gap-2">
          <StatusPill icon={Send} label="Lệnh từ xa qua MQTT/SIM (dashboard này)" tone="neutral" />
          <StatusPill icon={Radio} label="Nút vật lý trên thiết bị" tone="neutral" />
          <StatusPill icon={Activity} label="Tự động khi tốc độ vượt ngưỡng" tone="neutral" />
        </div>
      </Card>

      <Card style={{ marginBottom: 16 }}>
        <StepHeader number="1" title="Kiểm tra hệ thống trước bay" done={allPass} />
        <div>
          {TEST_ITEMS.map((it) => (
            <TestItemRow key={it.key} item={it} status={tests[it.key]} onRun={() => runTest(it.key)} />
          ))}
        </div>
        <button
          onClick={runAllTests}
          className="mt-4 flex items-center gap-2 text-xs font-medium px-3 py-2 rounded-lg"
          style={{ background: C.navy, color: "#fff" }}
        >
          <ClipboardCheck size={14} /> Chạy tất cả kiểm tra
        </button>
      </Card>

      <Card style={{ marginBottom: 16 }}>
        <StepHeader number="2" title="Thiết lập trước chuyến bay" done={setupDone} />
        <div className="grid grid-cols-2 gap-4">
          <div>
            <label className="text-[11px]" style={{ color: C.inkFaint }}>Tên tệp log</label>
            <input
              value={setup.filename}
              onChange={(e) => setSetup((s) => ({ ...s, filename: e.target.value }))}
              className="w-full mt-1 px-3 py-2 rounded-lg text-xs"
              style={{ border: `1px solid ${C.border}`, fontFamily: FONT_MONO, color: C.ink }}
              placeholder="log_YYYYMMDD_ten_chuyen_bay.csv"
            />
          </div>
          <div>
            <label className="text-[11px]" style={{ color: C.inkFaint }}>Người vận hành / Phi công</label>
            <input
              value={setup.pilot}
              onChange={(e) => setSetup((s) => ({ ...s, pilot: e.target.value }))}
              className="w-full mt-1 px-3 py-2 rounded-lg text-xs"
              style={{ border: `1px solid ${C.border}`, color: C.ink }}
              placeholder="Nhập tên"
            />
          </div>
          <div>
            <label className="text-[11px]" style={{ color: C.inkFaint }}>Mã máy bay / thiết bị</label>
            <input
              value={setup.aircraftId}
              onChange={(e) => setSetup((s) => ({ ...s, aircraftId: e.target.value }))}
              className="w-full mt-1 px-3 py-2 rounded-lg text-xs"
              style={{ border: `1px solid ${C.border}`, fontFamily: FONT_MONO, color: C.ink }}
            />
          </div>
          <div className="flex items-center gap-2" style={{ marginTop: 20 }}>
            <input
              type="checkbox"
              id="autoTrigger"
              checked={autoTrigger}
              onChange={(e) => setAutoTrigger(e.target.checked)}
            />
            <label htmlFor="autoTrigger" className="text-xs" style={{ color: C.inkSoft }}>
              Tự động bắt đầu khi tốc độ &gt; 5 km/h
            </label>
          </div>
          <div style={{ gridColumn: "span 2" }}>
            <label className="text-[11px]" style={{ color: C.inkFaint }}>Ghi chú chuyến bay</label>
            <input
              value={setup.notes}
              onChange={(e) => setSetup((s) => ({ ...s, notes: e.target.value }))}
              className="w-full mt-1 px-3 py-2 rounded-lg text-xs"
              style={{ border: `1px solid ${C.border}`, color: C.ink }}
              placeholder="Tùy chọn"
            />
          </div>
        </div>
      </Card>

      <Card style={{ marginBottom: 16 }}>
        <StepHeader number="3" title="Ra tín hiệu bay" done={flightState === "flying" || flightState === "ended"} />
        <div className="flex items-center justify-between">
          <div>
            <div className="text-xs" style={{ color: C.inkSoft }}>Trạng thái hiện tại</div>
            <div className="text-base font-semibold" style={{ color: C.ink }}>{flightLabel}</div>
            {!canStart && flightState !== "flying" && (
              <div className="text-[11px] mt-1" style={{ color: C.amber }}>
                {!testsDone ? "Cần hoàn tất kiểm tra hệ thống ở Bước 1" : "Cần nhập tên tệp và người vận hành ở Bước 2"}
              </div>
            )}
          </div>
          <StatusPill icon={Rocket} label={flightState.toUpperCase()} tone={flightTone} />
        </div>
        <div className="flex items-center gap-3 mt-4">
          {flightState !== "flying" ? (
            <button
              onClick={startFlight}
              disabled={!canStart}
              className="flex items-center gap-2 text-sm font-semibold px-5 py-3 rounded-xl"
              style={{ background: canStart ? C.green : C.border, color: canStart ? "#fff" : C.inkFaint }}
            >
              <Rocket size={16} /> Bắt đầu chuyến bay
            </button>
          ) : (
            <button
              onClick={endFlight}
              className="flex items-center gap-2 text-sm font-semibold px-5 py-3 rounded-xl"
              style={{ background: C.red, color: "#fff" }}
            >
              <StopCircle size={16} /> Kết thúc chuyến bay
            </button>
          )}
          {flightState === "ended" && (
            <button
              onClick={newFlight}
              className="flex items-center gap-2 text-xs font-medium px-4 py-2.5 rounded-xl"
              style={{ border: `1px solid ${C.border}`, color: C.inkSoft }}
            >
              <RefreshCw size={13} /> Thiết lập chuyến bay mới
            </button>
          )}
        </div>
      </Card>

      <Card>
        <StepHeader number="4" title="Gửi cảnh báo & yêu cầu khi bay" done={false} />
        <div className="text-[11px] mb-3" style={{ color: C.inkFaint }}>
          Gửi qua MQTT/SIM tới trạm mặt đất — khả dụng bất kỳ lúc nào, nên dùng trong khi đang bay
        </div>
        <div className="grid grid-cols-4 gap-2 mb-4">
          {ALERT_TYPES.map((a) => (
            <button
              key={a.type}
              onClick={() => sendAlert(a)}
              className="flex flex-col items-center gap-1.5 text-center py-3 rounded-lg"
              style={{ border: `1px solid ${C.border}`, background: C.surface }}
            >
              <a.icon size={16} style={{ color: a.tone === "bad" ? C.red : a.tone === "warn" ? C.amber : C.teal }} />
              <span className="text-[11px] font-medium" style={{ color: C.inkSoft }}>{a.label}</span>
            </button>
          ))}
        </div>
        {alertLog.length > 0 && (
          <div className="space-y-2 pt-2" style={{ borderTop: `1px solid ${C.border}` }}>
            {alertLog.slice(0, 6).map((a, i) => (
              <div key={i} className="flex items-center gap-2">
                <div className="rounded-full" style={{ width: 6, height: 6, background: a.level === "ok" ? C.green : C.amber }} />
                <span className="text-xs" style={{ color: C.ink }}>{a.msg}</span>
                <span className="text-[11px] ml-auto" style={{ color: C.inkFaint, fontFamily: FONT_MONO }}>{a.time}</span>
              </div>
            ))}
          </div>
        )}
      </Card>
    </div>
  );
}

function FlightNoticeBar({ flightState, setActiveSection }) {
  const config = {
    flying: { bg: C.greenSoft, fg: C.green, icon: Rocket, text: "Đang có chuyến bay diễn ra — thiết bị đang ghi log telemetry" },
    ready: { bg: C.navySoft, fg: C.navy, icon: ClipboardCheck, text: "Đã sẵn sàng — chưa có chuyến bay nào đang diễn ra" },
    ended: { bg: C.amberSoft, fg: C.amber, icon: AlertTriangle, text: "Không có chuyến bay nào đang diễn ra — chuyến gần nhất đã kết thúc" },
    idle: { bg: C.bg, fg: C.inkSoft, icon: Circle, text: "Không có chuyến bay nào đang diễn ra" },
  }[flightState];
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
          className="text-[11px] font-medium"
          style={{ color: config.fg, textDecoration: "underline" }}
        >
          Đến Điều khiển chuyến bay
        </button>
      )}
    </div>
  );
}

export default function App() {
  const [activeSection, setActiveSection] = useState("overview");
  const [buzzerStatus, setBuzzerStatus] = useState("Chờ lệnh (Standby)");
  const [flightState, setFlightState] = useState("idle");

  const runBuzzerTest = (label) => {
    setBuzzerStatus(label);
    setTimeout(() => setBuzzerStatus("Chờ lệnh (Standby)"), 1600);
  };

  return (
    <div className="flex" style={{ minHeight: 640, background: C.bg, fontFamily: FONT_SANS }}>
      <style>{`@import url('https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500;600;700&family=Inter:wght@400;500;600;700&display=swap');`}</style>

      <aside className="flex flex-col justify-between" style={{ width: 248, background: C.surface, borderRight: `1px solid ${C.border}` }}>
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
                  className="w-full flex items-center gap-3 px-3 py-2 rounded-lg text-left"
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
        <div className="p-4" style={{ borderTop: `1px solid ${C.border}` }}>
          <DeviceScreenPreview section={activeSection} flightState={flightState} />
        </div>
      </aside>

      <main className="flex-1 overflow-y-auto">
        <div className="sticky top-0 z-10 flex items-center justify-between px-8 py-3" style={{ background: "rgba(247,248,250,0.9)", backdropFilter: "blur(6px)", borderBottom: `1px solid ${C.border}` }}>
          <div className="text-sm font-medium" style={{ color: C.inkSoft }}>
            Bảng điều khiển / <span style={{ color: C.ink }}>{NAV_ITEMS.find((n) => n.id === activeSection)?.label}</span>
          </div>
          <div className="flex items-center gap-2">
            <StatusPill icon={Rocket} label={flightState === "flying" ? "Đang bay" : flightState === "ended" ? "Đã kết thúc" : "Chờ"} tone={flightState === "flying" ? "ok" : "neutral"} />
            <StatusPill icon={Database} label="SD" value="OK" tone="ok" />
            <StatusPill icon={Wifi} label="SIM" value="OK" tone="ok" />
            <StatusPill icon={Battery} label="Pin" value="82%" tone="neutral" />
          </div>
        </div>

        <FlightNoticeBar flightState={flightState} setActiveSection={setActiveSection} />

        <div className="px-8 py-8">
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
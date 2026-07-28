export function parseDurationToMinutes(str) {
    if (!str) return 0;
    const hMatch = str.match(/(\d+)\s*h/);
    const mMatch = str.match(/(\d+)\s*p/);
    const h = hMatch ? parseInt(hMatch[1], 10) : 0;
    const m = mMatch ? parseInt(mMatch[1], 10) : 0;
    return h * 60 + (m || (h === 0 ? parseInt(str, 10) || 0 : 0));
}

export function formatTime(sec) {
    const s = Math.max(0, Math.round(sec));
    const mm = Math.floor(s / 60);
    const ss = s % 60;
    return `${String(mm).padStart(2, "0")}:${String(ss).padStart(2, "0")}`;
}

export function generateTrack(flight) {
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

export function attitudeStatus(pitch, roll) {
    const p = Math.abs(pitch), r = Math.abs(roll);
    if (p > 35 || r > 45) return { tone: "bad", label: "Vượt ngưỡng an toàn" };
    if (p > 20 || r > 25) return { tone: "warn", label: "Gần ngưỡng cảnh báo" };
    return { tone: "ok", label: "Trong ngưỡng bình thường" };
}

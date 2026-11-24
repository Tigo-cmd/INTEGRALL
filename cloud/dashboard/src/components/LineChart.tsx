// Minimal SVG line chart (no chart library). One metric at a time, each with
// its own Y scale — temperature (~24) and RSSI (~-55) on one axis would both
// look flat.
import { useEffect, useMemo, useState } from "react";
import type { TelemetryRow } from "../lib/api";

export default function LineChart({ rows, keys }: { rows: TelemetryRow[]; keys: string[] }) {
  const [key, setKey] = useState<string | null>(null);
  useEffect(() => {
    if (!key || !keys.includes(key)) setKey(keys[0] ?? null);
  }, [keys, key]);

  const pts = useMemo(() => (key ? rows
    .map((r) => ({ t: new Date(r.ts).getTime(), v: r.data[key] }))
    .filter((p): p is { t: number; v: number } => typeof p.v === "number" && Number.isFinite(p.v)) : []), [rows, key]);

  if (!keys.length || !key) {
    return <p className="py-12 text-center text-sm text-slate-500">No numeric telemetry in this window yet.</p>;
  }

  const W = 800, H = 240, P = { l: 48, r: 12, t: 10, b: 24 };
  const t0 = Math.min(...pts.map((p) => p.t)), t1 = Math.max(...pts.map((p) => p.t));
  let v0 = Math.min(...pts.map((p) => p.v)), v1 = Math.max(...pts.map((p) => p.v));
  const pad = (v1 - v0) * 0.1 || 1;
  v0 -= pad; v1 += pad;
  const x = (t: number) => P.l + ((t - t0) / Math.max(1, t1 - t0)) * (W - P.l - P.r);
  const y = (v: number) => P.t + (1 - (v - v0) / (v1 - v0)) * (H - P.t - P.b);
  const ticks = [v0 + pad, (v0 + v1) / 2, v1 - pad];
  const time = (t: number) => new Date(t).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
  const last = pts[pts.length - 1];

  return (
    <div>
      <div className="mb-2 flex flex-wrap gap-1" role="tablist" aria-label="Metric">
        {keys.map((k) => (
          <button key={k} role="tab" aria-selected={k === key} onClick={() => setKey(k)}
                  className={`rounded-md px-2 py-1 font-mono text-xs ${k === key ? "bg-accent/15 text-accent" : "text-slate-500 hover:text-slate-300"}`}>
            {k}
          </button>
        ))}
      </div>
      {pts.length < 2 ? (
        <p className="py-12 text-center text-sm text-slate-500">Waiting for more {key} samples…</p>
      ) : (
        <svg viewBox={`0 0 ${W} ${H}`} className="w-full" role="img"
             aria-label={`${key} over time, latest ${last.v}`}>
          {ticks.map((v, i) => (
            <g key={i}>
              <line x1={P.l} x2={W - P.r} y1={y(v)} y2={y(v)} stroke="#1b2330" />
              <text x={P.l - 6} y={y(v) + 4} textAnchor="end" fontSize="11" fill="#64748b">{Number(v.toFixed(1))}</text>
            </g>
          ))}
          <text x={P.l} y={H - 6} fontSize="11" fill="#64748b">{time(t0)}</text>
          <text x={W - P.r} y={H - 6} fontSize="11" fill="#64748b" textAnchor="end">{time(t1)}</text>
          <polyline fill="none" stroke="#22d3ee" strokeWidth="2" strokeLinejoin="round"
                    points={pts.map((p) => `${x(p.t)},${y(p.v)}`).join(" ")} />
          <circle cx={x(last.t)} cy={y(last.v)} r="3.5" fill="#22d3ee" />
        </svg>
      )}
    </div>
  );
}

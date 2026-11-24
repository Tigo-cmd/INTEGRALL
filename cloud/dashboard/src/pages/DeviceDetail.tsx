import { useCallback, useEffect, useMemo, useState } from "react";
import { useNavigate, useParams } from "react-router-dom";
import { api, Command, Device, EventRow, TelemetryRow } from "../lib/api";
import { live } from "../lib/live";
import { ago, fmtValue, uptime } from "../lib/format";
import StatusDot from "../components/StatusDot";
import LineChart from "../components/LineChart";
import Modal from "../components/Modal";
import TokenReveal from "../components/TokenReveal";

const WINDOWS = [{ label: "15m", m: 15 }, { label: "1h", m: 60 }, { label: "24h", m: 1440 }];
const MAX_POINTS = 2000;

export default function DeviceDetail() {
  const { id = "" } = useParams();
  const nav = useNavigate();
  const [device, setDevice] = useState<Device | null>(null);
  const [rows, setRows] = useState<TelemetryRow[]>([]);
  const [events, setEvents] = useState<EventRow[]>([]);
  const [commands, setCommands] = useState<Command[]>([]);
  const [minutes, setMinutes] = useState(60);
  const [error, setError] = useState("");
  const [newToken, setNewToken] = useState<string | null>(null);

  const load = useCallback(async () => {
    try {
      const [d, t, e, c] = await Promise.all([api.device(id), api.telemetry(id, minutes), api.events(id), api.commands(id)]);
      setDevice(d); setRows(t); setEvents(e); setCommands(c); setError("");
    } catch (err) {
      setError((err as Error).message);
    }
  }, [id, minutes]);
  useEffect(() => { load(); }, [load]);

  useEffect(() => live.subscribe((m) => {
    if (m.type === "hello") { load(); return; }
    if (!("device_id" in m) || m.device_id !== id) return;
    const now = new Date().toISOString();
    switch (m.type) {
      case "status": setDevice((d) => d && { ...d, online: m.online, last_seen: now }); break;
      case "meta": setDevice((d) => d && { ...d, meta: m.meta as Device["meta"], last_seen: now }); break;
      case "health": setDevice((d) => d && { ...d, health: m.health as Device["health"], last_seen: now }); break;
      case "telemetry":
        setDevice((d) => d && { ...d, online: true, last_seen: now, last_telemetry: m.data });
        setRows((r) => [...r, { ts: m.ts, data: m.data }].slice(-MAX_POINTS));
        break;
      case "event": setEvents((e) => [{ ts: m.ts, type: m.event, detail: m.detail }, ...e].slice(0, 50)); break;
      case "command": {
        // The ack can arrive before the POST response that created the command.
        setCommands((cs) => cs.some((c) => c.id === m.command.id)
          ? cs.map((c) => (c.id === m.command.id ? m.command : c))
          : [m.command, ...cs].slice(0, 20));
        // reflect relay state from acks without waiting for the next telemetry
        const res = m.command.result as { on?: boolean } | null;
        const idx = (m.command.args as { index?: number }).index;
        if (m.command.status === "acked" && m.command.action.startsWith("relay.") && typeof res?.on === "boolean") {
          setDevice((d) => d && d.meta?.relays ? { ...d, meta: { ...d.meta, relays: d.meta.relays.map((r) => r.index === idx ? { ...r, on: res.on! } : r) } } : d);
        }
        break;
      }
    }
  }), [id, load]);

  const numericKeys = useMemo(() => {
    const ks = new Set<string>();
    rows.forEach((r) => Object.entries(r.data).forEach(([k, v]) => typeof v === "number" && ks.add(k)));
    return [...ks].sort();
  }, [rows]);

  async function send(action: string, args: Record<string, unknown> = {}) {
    try {
      const c = await api.command(id, action, args);
      setCommands((cs) => (cs.some((x) => x.id === c.id) ? cs : [c, ...cs].slice(0, 20)));   // keep a faster ack
    } catch (err) {
      setError((err as Error).message);
    }
  }

  async function remove() {
    if (!device || !window.confirm(`Delete ${device.name}? Its broker login and history are removed.`)) return;
    await api.deleteDevice(id);
    nav("/");
  }

  if (error && !device) return <p role="alert" className="text-red-300">{error}</p>;
  if (!device) return <p className="text-slate-500">Loading…</p>;
  const meta = device.meta ?? {};
  const health = device.health ?? {};
  const pending = new Set(commands.filter((c) => c.status === "sent").map((c) => `${c.action}:${JSON.stringify(c.args)}`));

  return (
    <div className="space-y-5">
      <div className="flex flex-wrap items-start justify-between gap-3">
        <div>
          <h1 className="text-2xl font-semibold text-slate-100">{device.name}</h1>
          <div className="mt-1 flex flex-wrap items-center gap-3 font-mono text-xs text-slate-500">
            <span>{device.id}</span>
            <StatusDot online={device.online} />
            <span>seen {ago(device.last_seen)}</span>
            {meta.fw && <span>fw {meta.fw}</span>}
            {meta.chip && <span>{meta.chip}</span>}
            {meta.ip && <span>{meta.ip}</span>}
          </div>
        </div>
        <div className="flex flex-wrap gap-2">
          <button className="btn-ghost" disabled={!device.online} onClick={() => send("ping")}>Ping</button>
          <button className="btn-ghost" disabled={!device.online} onClick={() => send("health")}>Run self-test</button>
          <button className="btn-ghost" disabled={!device.online} onClick={() => send("reboot")}>Reboot</button>
          <button className="btn-ghost" onClick={async () => setNewToken((await api.rotateToken(id)).token)}>Rotate token</button>
          <button className="btn-danger" onClick={remove}>Delete</button>
        </div>
      </div>
      {error && <p role="alert" className="rounded-lg bg-red-500/10 px-3 py-2 text-sm text-red-300">{error}</p>}

      <div className="grid gap-4 lg:grid-cols-3">
        <section className="card lg:col-span-2">
          <div className="mb-3 flex items-center justify-between">
            <h2 className="label">Telemetry</h2>
            <div className="flex gap-1" role="group" aria-label="Time window">
              {WINDOWS.map((w) => (
                <button key={w.m} onClick={() => setMinutes(w.m)} aria-pressed={minutes === w.m}
                        className={`rounded-md px-2 py-1 text-xs ${minutes === w.m ? "bg-ink-700 text-slate-100" : "text-slate-500 hover:text-slate-300"}`}>{w.label}</button>
              ))}
            </div>
          </div>
          <LineChart rows={rows} keys={numericKeys} />
        </section>

        <section className="card space-y-3">
          <h2 className="label">Latest values</h2>
          {device.last_telemetry ? (
            <dl className="grid grid-cols-2 gap-2">
              {Object.entries(device.last_telemetry).map(([k, v]) => (
                <div key={k} className="rounded-lg bg-ink-950 px-2 py-1.5">
                  <dt className="truncate font-mono text-[11px] text-slate-500">{k}</dt>
                  <dd className="font-mono text-lg text-slate-100">{fmtValue(v)}</dd>
                </div>
              ))}
            </dl>
          ) : <p className="text-sm text-slate-500">Nothing received yet.</p>}
        </section>
      </div>

      <div className="grid gap-4 lg:grid-cols-3">
        <section className="card space-y-3">
          <h2 className="label">Controls</h2>
          {meta.relays?.length ? meta.relays.map((r) => {
            const busy = pending.has(`relay.set:${JSON.stringify({ index: r.index, on: !r.on })}`);
            return (
              <div key={r.index} className="flex items-center justify-between">
                <span className="text-sm">{r.name || `Relay ${r.index}`}</span>
                <button role="switch" aria-checked={r.on} disabled={!device.online || busy}
                        onClick={() => send("relay.set", { index: r.index, on: !r.on })}
                        className={`relative h-6 w-11 rounded-full transition disabled:opacity-40 ${r.on ? "bg-accent" : "bg-ink-600"}`}>
                  <span className={`absolute top-0.5 h-5 w-5 rounded-full bg-white transition-all ${r.on ? "left-5" : "left-0.5"}`} />
                </button>
              </div>
            );
          }) : <p className="text-sm text-slate-500">No relays reported by this device.</p>}
          {meta.modules?.includes("buzzer") && (
            <button className="btn-ghost w-full justify-center" disabled={!device.online} onClick={() => send("buzzer.beep", { ms: 300 })}>Beep</button>
          )}
        </section>

        <section className="card space-y-3">
          <div className="flex items-center justify-between">
            <h2 className="label">Hardware self-test</h2>
            <span className="font-mono text-xs text-slate-500">up {uptime(health.uptime_s)}</span>
          </div>
          {health.parts ? (
            <ul className="space-y-1.5">
              {Object.entries(health.parts).map(([part, ok]) => (
                <li key={part} className="flex items-center justify-between text-sm">
                  <span className="font-mono">{part}</span>
                  <span className={ok ? "text-emerald-400" : "text-red-400"}>{ok ? "connected" : "missing"}</span>
                </li>
              ))}
            </ul>
          ) : <p className="text-sm text-slate-500">No report yet.</p>}
          {health.i2c?.length ? <p className="font-mono text-xs text-slate-500">I2C: {health.i2c.join(" ")}</p> : null}
          {health.heap !== undefined && <p className="font-mono text-xs text-slate-500">free heap {health.heap} B{health.rssi !== undefined ? ` · RSSI ${health.rssi} dBm` : ""}</p>}
        </section>

        <section className="card space-y-2">
          <h2 className="label">Commands</h2>
          {commands.length ? (
            <ul className="space-y-1.5 text-sm">
              {commands.slice(0, 8).map((c) => (
                <li key={c.id} className="flex items-center justify-between gap-2">
                  <span className="truncate font-mono text-xs">{c.action}</span>
                  <span title={c.message ?? ""} className={`rounded px-1.5 py-0.5 text-[11px] font-semibold ${
                    c.status === "acked" ? "bg-emerald-500/15 text-emerald-300" :
                    c.status === "sent" ? "bg-accent/15 text-accent" : "bg-red-500/15 text-red-300"}`}>{c.status}</span>
                </li>
              ))}
            </ul>
          ) : <p className="text-sm text-slate-500">None yet.</p>}
        </section>
      </div>

      <section className="card">
        <h2 className="label mb-2">Events</h2>
        {events.length ? (
          <ul className="divide-y divide-ink-700 text-sm">
            {events.map((e, i) => (
              <li key={`${e.ts}-${i}`} className="flex gap-3 py-1.5">
                <span className="w-24 shrink-0 font-mono text-xs text-slate-500">{new Date(e.ts).toLocaleTimeString()}</span>
                <span className="font-mono text-xs text-accent">{e.type}</span>
                <span className="text-slate-300">{e.detail}</span>
              </li>
            ))}
          </ul>
        ) : <p className="text-sm text-slate-500">No events.</p>}
      </section>

      {newToken && (
        <Modal title="New device token" onClose={() => setNewToken(null)}>
          <TokenReveal deviceId={device.id} token={newToken} />
          <div className="text-right"><button className="btn-primary" onClick={() => setNewToken(null)}>Done</button></div>
        </Modal>
      )}
    </div>
  );
}

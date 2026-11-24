import { FormEvent, useCallback, useEffect, useState } from "react";
import { Link } from "react-router-dom";
import { api, Device } from "../lib/api";
import { live } from "../lib/live";
import { ago, fmtValue } from "../lib/format";
import StatusDot from "../components/StatusDot";
import Modal from "../components/Modal";
import TokenReveal from "../components/TokenReveal";

export default function Devices() {
  const [devices, setDevices] = useState<Device[] | null>(null);
  const [error, setError] = useState("");
  const [adding, setAdding] = useState(false);

  const load = useCallback(() => api.devices().then(setDevices).catch((e) => setError(e.message)), []);
  useEffect(() => { load(); }, [load]);

  // Live updates patch the list in place.
  useEffect(() => live.subscribe((m) => {
    if (m.type === "hello") { load(); return; }                    // resync after reconnect
    if (!("device_id" in m)) return;
    setDevices((ds) => ds && ds.map((d) => {
      if (d.id !== m.device_id) return d;
      const now = new Date().toISOString();
      switch (m.type) {
        case "status": return { ...d, online: m.online, last_seen: now };
        case "telemetry": return { ...d, online: true, last_seen: now, last_telemetry: m.data };
        case "meta": return { ...d, meta: m.meta as Device["meta"], last_seen: now };
        case "health": return { ...d, health: m.health as Device["health"], last_seen: now };
        default: return { ...d, last_seen: now };
      }
    }));
  }), [load]);

  return (
    <div className="space-y-5">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-semibold text-slate-100">Devices</h1>
        <button className="btn-primary" onClick={() => setAdding(true)}>+ Add device</button>
      </div>
      {error && <p role="alert" className="text-sm text-red-300">{error}</p>}
      {devices === null ? (
        <p className="text-slate-500">Loading…</p>
      ) : devices.length === 0 ? (
        <div className="card py-12 text-center">
          <p className="text-slate-300">No devices yet.</p>
          <p className="mt-1 text-sm text-slate-500">Flash the <code className="font-mono">CloudMQTT</code> example, copy the Device ID from the serial monitor, then add it here.</p>
        </div>
      ) : (
        <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-3">
          {devices.map((d) => (
            <Link key={d.id} to={`/devices/${d.id}`} className="card block transition hover:border-accent/60">
              <div className="flex items-start justify-between gap-2">
                <div className="min-w-0">
                  <div className="truncate font-semibold text-slate-100">{d.name}</div>
                  <div className="font-mono text-xs text-slate-500">{d.id}</div>
                </div>
                <StatusDot online={d.online} />
              </div>
              <div className="mt-4 grid grid-cols-2 gap-2">
                {Object.entries(d.last_telemetry ?? {}).slice(0, 4).map(([k, v]) => (
                  <div key={k} className="rounded-lg bg-ink-950 px-2 py-1.5">
                    <div className="truncate font-mono text-[11px] text-slate-500">{k}</div>
                    <div className="font-mono text-sm text-slate-200">{fmtValue(v)}</div>
                  </div>
                ))}
              </div>
              <div className="mt-3 text-xs text-slate-500">
                seen {ago(d.last_seen)}{d.health?.missing ? <span className="ml-2 text-amber-400">· {d.health.missing} part(s) missing</span> : null}
              </div>
            </Link>
          ))}
        </div>
      )}
      {adding && <AddDevice onClose={() => { setAdding(false); load(); }} />}
    </div>
  );
}

function AddDevice({ onClose }: { onClose: () => void }) {
  const [id, setId] = useState("");
  const [name, setName] = useState("");
  const [error, setError] = useState("");
  const [busy, setBusy] = useState(false);
  const [created, setCreated] = useState<{ id: string; token: string } | null>(null);

  async function submit(e: FormEvent) {
    e.preventDefault();
    setBusy(true);
    setError("");
    try {
      const d = await api.createDevice(id.trim(), name.trim());
      setCreated({ id: d.id, token: d.token });
    } catch (err) {
      setError((err as Error).message);
    } finally {
      setBusy(false);
    }
  }

  return (
    <Modal title={created ? "Device added" : "Add device"} onClose={onClose}>
      {created ? (
        <>
          <TokenReveal deviceId={created.id} token={created.token} />
          <div className="text-right"><button className="btn-primary" onClick={onClose}>Done</button></div>
        </>
      ) : (
        <form onSubmit={submit} className="space-y-3">
          <label className="block space-y-1">
            <span className="label">Device ID</span>
            <input className="input font-mono" placeholder="INT_A1B2C3D4E5F6" required value={id}
                   pattern="[Ii][Nn][Tt]_[0-9A-Fa-f]{12}" onChange={(e) => setId(e.target.value)} />
            <span className="text-xs text-slate-500">Printed by the board at boot ("Device ID: …").</span>
          </label>
          <label className="block space-y-1">
            <span className="label">Name</span>
            <input className="input" placeholder="Greenhouse pump" required maxLength={80} value={name} onChange={(e) => setName(e.target.value)} />
          </label>
          {error && <p role="alert" className="rounded-lg bg-red-500/10 px-3 py-2 text-sm text-red-300">{error}</p>}
          <div className="flex justify-end gap-2">
            <button type="button" className="btn-ghost" onClick={onClose}>Cancel</button>
            <button className="btn-primary" disabled={busy}>{busy ? "Adding…" : "Add device"}</button>
          </div>
        </form>
      )}
    </Modal>
  );
}

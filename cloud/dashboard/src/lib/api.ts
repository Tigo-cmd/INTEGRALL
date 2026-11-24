// Thin REST client. The JWT lives in sessionStorage (cleared when the tab closes).
export type Device = {
  id: string;
  name: string;
  online: boolean;
  last_seen: string | null;
  meta: Meta | null;
  health: Health | null;
  last_telemetry: Record<string, unknown> | null;
  created_at: string;
};
export type Meta = {
  fw?: string; chip?: string; board?: string; ip?: string; mac?: string;
  modules?: string[];
  relays?: { index: number; name: string; on: boolean }[];
};
export type Health = { parts?: Record<string, boolean>; i2c?: string[]; heap?: number; uptime_s?: number; rssi?: number; missing?: number };
export type TelemetryRow = { ts: string; data: Record<string, unknown> };
export type EventRow = { ts: string; type: string; detail: string };
export type Command = {
  id: string; device_id?: string; action: string; args: Record<string, unknown>;
  status: "sent" | "acked" | "failed" | "expired"; message: string | null;
  result: Record<string, unknown> | null; created_at: string; completed_at: string | null;
};

const TOKEN_KEY = "integrall.jwt";
export const auth = {
  get: () => sessionStorage.getItem(TOKEN_KEY),
  set: (t: string) => sessionStorage.setItem(TOKEN_KEY, t),
  clear: () => sessionStorage.removeItem(TOKEN_KEY),
};

export class ApiError extends Error {
  constructor(public status: number, message: string) { super(message); }
}

async function req<T>(method: string, path: string, body?: unknown): Promise<T> {
  const headers: Record<string, string> = {};
  const t = auth.get();
  if (t) headers.Authorization = `Bearer ${t}`;
  if (body !== undefined) headers["Content-Type"] = "application/json";
  const r = await fetch(path, { method, headers, body: body === undefined ? undefined : JSON.stringify(body) });
  if (r.status === 401) { auth.clear(); window.dispatchEvent(new Event("integrall:logout")); }
  if (!r.ok) {
    let msg = r.statusText;
    try {
      const j = await r.json();
      msg = typeof j.detail === "string" ? j.detail : Array.isArray(j.detail) ? j.detail.map((d: { msg: string }) => d.msg).join("; ") : msg;
    } catch { /* not JSON */ }
    throw new ApiError(r.status, msg);
  }
  return r.status === 204 ? (undefined as T) : r.json();
}

export const api = {
  register: (email: string, password: string) => req<{ access_token: string }>("POST", "/api/auth/register", { email, password }),
  login: (email: string, password: string) => req<{ access_token: string }>("POST", "/api/auth/login", { email, password }),
  me: () => req<{ id: number; email: string }>("GET", "/api/auth/me"),
  devices: () => req<Device[]>("GET", "/api/devices"),
  device: (id: string) => req<Device>("GET", `/api/devices/${id}`),
  createDevice: (id: string, name: string) => req<Device & { token: string }>("POST", "/api/devices", { id, name }),
  deleteDevice: (id: string) => req<void>("DELETE", `/api/devices/${id}`),
  rotateToken: (id: string) => req<{ token: string }>("POST", `/api/devices/${id}/rotate-token`),
  telemetry: (id: string, minutes = 60) => req<TelemetryRow[]>("GET", `/api/devices/${id}/telemetry?minutes=${minutes}`),
  events: (id: string) => req<EventRow[]>("GET", `/api/devices/${id}/events?limit=50`),
  commands: (id: string) => req<Command[]>("GET", `/api/devices/${id}/commands?limit=20`),
  command: (id: string, action: string, args: Record<string, unknown> = {}) =>
    req<Command>("POST", `/api/devices/${id}/commands`, { action, args }),
};

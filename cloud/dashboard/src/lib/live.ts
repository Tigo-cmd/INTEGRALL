// One WebSocket per tab; the server pushes every change (nothing is polled).
import { auth } from "./api";

export type LiveMessage =
  | { type: "hello"; user: string }
  | { type: "status"; device_id: string; online: boolean }
  | { type: "meta"; device_id: string; meta: Record<string, unknown> }
  | { type: "health"; device_id: string; health: Record<string, unknown> }
  | { type: "telemetry"; device_id: string; ts: string; data: Record<string, unknown> }
  | { type: "event"; device_id: string; ts: string; event: string; detail: string }
  | { type: "command"; device_id: string; command: import("./api").Command };

type Listener = (m: LiveMessage) => void;

class Live {
  private ws: WebSocket | null = null;
  private listeners = new Set<Listener>();
  private retry = 1000;
  private stopped = true;
  private keepAlive: number | undefined;
  connected = false;
  onState: ((connected: boolean) => void) | null = null;

  start() {
    if (!this.stopped) return;
    this.stopped = false;
    this.open();
  }

  stop() {
    this.stopped = true;
    window.clearInterval(this.keepAlive);
    this.ws?.close();
    this.ws = null;
  }

  subscribe(fn: Listener) {
    this.listeners.add(fn);
    return () => { this.listeners.delete(fn); };
  }

  private setConnected(v: boolean) { this.connected = v; this.onState?.(v); }

  private open() {
    const token = auth.get();
    if (!token || this.stopped) return;
    const url = `${location.protocol === "https:" ? "wss" : "ws"}://${location.host}/api/ws`;
    const ws = new WebSocket(url);
    this.ws = ws;
    ws.onopen = () => ws.send(JSON.stringify({ token }));     // token in first frame, not in the URL
    ws.onmessage = (e) => {
      const msg = JSON.parse(e.data) as LiveMessage;
      if (msg.type === "hello") { this.retry = 1000; this.setConnected(true); }
      this.listeners.forEach((fn) => fn(msg));
    };
    ws.onclose = () => {
      this.setConnected(false);
      window.clearInterval(this.keepAlive);
      if (this.stopped) return;
      window.setTimeout(() => this.open(), this.retry);        // reconnect with back-off
      this.retry = Math.min(this.retry * 2, 30000);
    };
    // keep idle proxies from dropping the socket
    this.keepAlive = window.setInterval(() => ws.readyState === 1 && ws.send("ping"), 25000);
  }
}

export const live = new Live();

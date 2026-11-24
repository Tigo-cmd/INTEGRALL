import { useEffect, useState } from "react";
import { Link, Navigate, Route, Routes, useNavigate } from "react-router-dom";
import { api, auth } from "./lib/api";
import { live } from "./lib/live";
import Login from "./pages/Login";
import Devices from "./pages/Devices";
import DeviceDetail from "./pages/DeviceDetail";

export default function App() {
  const [email, setEmail] = useState<string | null>(null);
  const [checked, setChecked] = useState(false);
  const [liveUp, setLiveUp] = useState(false);
  const nav = useNavigate();

  useEffect(() => {
    live.onState = setLiveUp;
    const onLogout = () => { live.stop(); setEmail(null); nav("/login"); };
    window.addEventListener("integrall:logout", onLogout);
    if (auth.get()) {
      api.me().then((u) => { setEmail(u.email); live.start(); }).catch(() => auth.clear()).finally(() => setChecked(true));
    } else {
      setChecked(true);
    }
    return () => window.removeEventListener("integrall:logout", onLogout);
  }, [nav]);

  if (!checked) return null;

  const onLogin = (e: string) => { setEmail(e); live.start(); nav("/"); };
  const logout = () => { auth.clear(); window.dispatchEvent(new Event("integrall:logout")); };

  if (!email) {
    return (
      <Routes>
        <Route path="*" element={<Login onLogin={onLogin} />} />
      </Routes>
    );
  }

  return (
    <div className="min-h-screen text-slate-200">
      <header className="sticky top-0 z-10 border-b border-ink-700 bg-ink-950/90 backdrop-blur">
        <div className="mx-auto flex max-w-6xl items-center gap-4 px-4 py-3">
          <Link to="/" className="font-mono text-sm font-bold tracking-widest text-accent">INTEGRALL<span className="text-slate-500">/cloud</span></Link>
          <span className={`ml-2 flex items-center gap-1.5 text-xs ${liveUp ? "text-emerald-400" : "text-amber-400"}`}
                title={liveUp ? "Receiving live updates" : "Reconnecting live updates…"}>
            <span className={`h-2 w-2 rounded-full ${liveUp ? "bg-emerald-400" : "animate-pulse bg-amber-400"}`} />
            {liveUp ? "live" : "reconnecting"}
          </span>
          <div className="ml-auto flex items-center gap-3 text-sm">
            <span className="hidden text-slate-400 sm:inline">{email}</span>
            <button className="btn-ghost" onClick={logout}>Sign out</button>
          </div>
        </div>
      </header>
      <main className="mx-auto max-w-6xl px-4 py-6">
        <Routes>
          <Route path="/" element={<Devices />} />
          <Route path="/devices/:id" element={<DeviceDetail />} />
          <Route path="*" element={<Navigate to="/" replace />} />
        </Routes>
      </main>
    </div>
  );
}

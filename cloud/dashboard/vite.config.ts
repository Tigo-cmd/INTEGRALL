import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// In dev, /api (REST + WebSocket) is proxied to the backend (default :8000,
// override with INTEGRALL_API=http://host:port npm run dev).
const api = process.env.INTEGRALL_API ?? "http://localhost:8000";

export default defineConfig({
  plugins: [react()],
  server: {
    proxy: { "/api": { target: api, ws: true } },
  },
});

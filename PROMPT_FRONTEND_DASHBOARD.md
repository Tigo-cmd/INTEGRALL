# Frontend Engineering Prompt: Integrall Live Weather Dashboard

**Context:** You are building a professional, high-performance monitoring dashboard for the **Integrall Weather Station**. The system uses an ESP32 for sensing and a Flask API for data reception.

**Core Directives:**
- **Aesthetic**: Brutalist-modern. Think "Industrial Utility" meets "Swiss Precision." 
- **NO "Vibecoding"**: Avoid heavy neon gradients, excessive rounded corners, or glassmorphic "bubbles" that look like generic UI templates. 
- **Visual Language**: Clean layouts, high-contrast typography (Inter/JetBrains Mono), and sharp grid-based spacing. Use a "Dark Command Center" palette: Deep grays (`#121212`), high-contrast text (`#FFFFFF`), and "data accents" (e.g., `#00FF88` for success, `#FF3E3E` for alert).

**Dashboard Components:**
1.  **System Health Header**: 
    - Real-time "Online" status indicator with an actual pulse effect.
    - Large device ID and last-sync timestamp (formatted as `HH:MM:SS`).
    - Device uptime counter.

2.  **Live Sensor Matrix (The "Pulse"):**
    - 4 large primary data tiles for Temperature, Humidity, Pressure, and Light.
    - Each tile should have a "Rate of Change" indicator (e.g., `+1.2% in last 1h`).
    - Use clear, professional iconography (Lucide or Heroicons).

3.  **Real-Time Time-Series Charts (Mandatory):**
    - Use **Chart.js** or **Recharts**. 
    - **Chart 1 (Dual Axis)**: Temperature and Humidity over time. Use thin, crisp lines.
    - **Chart 2**: Bar chart for light/rain intensity levels over the last 15 points.
    - **Requirement**: No "bendy" curves. Use straight line segments or clean step charts to emphasize data precision over "look and feel."

4.  **Raw Telemetry Stream (The Log):**
    - A scrollable "Terminal" window at the bottom showing raw JSON strings from the backend as they arrive.
    - Use a monospaced font. Style it like a real engineering tool.

**Technical Constraints:**
- **Polling**: Implement efficient polling of `/api/live` every 3-5 seconds.
- **State Management**: Use React Hooks (useEffect, useState) for data ingestion. 
- **Responsive**: Grid layout that collapses into a vertical list on mobile.
- **Dependencies**: React, Tailwind CSS, Lucide-React, Recharts.

**Output Goal**: A single-page dashboard that feels like a mission control interface for an industrial sensor network—functional, fast, and authoritative.

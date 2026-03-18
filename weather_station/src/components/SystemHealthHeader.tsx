import { Activity } from 'lucide-react';
import { SystemStatus } from '../types';

interface SystemHealthHeaderProps {
  status: SystemStatus;
}

export default function SystemHealthHeader({ status }: SystemHealthHeaderProps) {
  const formatUptime = (seconds: number) => {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;

    if (days > 0) return `${days}d ${hours}h ${minutes}m`;
    if (hours > 0) return `${hours}h ${minutes}m ${secs}s`;
    return `${minutes}m ${secs}s`;
  };

  const formatTime = (timestamp: number) => {
    const date = new Date(timestamp);
    return date.toLocaleTimeString('en-US', { hour12: false });
  };

  return (
    <div className="border-b-2 border-white/10 pb-6 mb-6">
      <div className="flex items-start justify-between flex-wrap gap-4">
        <div>
          <div className="flex items-center gap-3 mb-2">
            <div className="relative">
              <Activity className="w-8 h-8 text-[#00FF88]" strokeWidth={2.5} />
              {status.online && (
                <span className="absolute -top-1 -right-1 w-3 h-3 bg-[#00FF88] rounded-full animate-pulse" />
              )}
            </div>
            <h1 className="text-3xl font-bold tracking-tight text-white uppercase">
              Integrall Weather Station
            </h1>
          </div>
          <div className="text-white/60 font-mono text-sm space-y-1">
            <div>DEVICE_ID: <span className="text-white">{status.deviceId}</span></div>
            <div>STATUS: <span className={status.online ? 'text-[#00FF88]' : 'text-[#FF3E3E]'}>
              {status.online ? 'ONLINE' : 'OFFLINE'}
            </span></div>
          </div>
        </div>

        <div className="text-right font-mono">
          <div className="text-white/40 text-xs uppercase tracking-wider mb-1">Last Sync</div>
          <div className="text-white text-2xl font-bold tabular-nums">{formatTime(status.lastSync)}</div>
          <div className="text-white/40 text-xs mt-2 uppercase tracking-wider">Uptime</div>
          <div className="text-white/80 text-sm tabular-nums">{formatUptime(status.uptime)}</div>
        </div>
      </div>
    </div>
  );
}

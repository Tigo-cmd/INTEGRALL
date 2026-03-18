import { useEffect, useRef } from 'react';
import { Terminal } from 'lucide-react';
import { TelemetryLog } from '../types';

interface TelemetryStreamProps {
  logs: TelemetryLog[];
}

export default function TelemetryStream({ logs }: TelemetryStreamProps) {
  const streamRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (streamRef.current) {
      streamRef.current.scrollTop = streamRef.current.scrollHeight;
    }
  }, [logs]);

  return (
    <div className="bg-black border-2 border-white/10 p-6">
      <div className="flex items-center gap-2 mb-4 border-b border-white/10 pb-3">
        <Terminal className="w-5 h-5 text-[#00FF88]" strokeWidth={2.5} />
        <h3 className="text-white font-mono text-sm uppercase tracking-widest">
          RAW TELEMETRY STREAM
        </h3>
        <div className="ml-auto text-white/40 font-mono text-xs">
          {logs.length} ENTRIES
        </div>
      </div>

      <div
        ref={streamRef}
        className="font-mono text-xs text-[#00FF88] space-y-1 max-h-64 overflow-y-auto scrollbar-thin scrollbar-thumb-white/20 scrollbar-track-transparent"
      >
        {logs.length === 0 ? (
          <div className="text-white/40">Waiting for telemetry data...</div>
        ) : (
          logs.map((log) => (
            <div key={log.id} className="flex gap-3">
              <span className="text-white/40">
                [{new Date(log.timestamp).toLocaleTimeString('en-US', { hour12: false })}]
              </span>
              <span className="text-[#00FF88]">
                {JSON.stringify(log.data)}
              </span>
            </div>
          ))
        )}
      </div>
    </div>
  );
}

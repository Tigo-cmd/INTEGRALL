import { useEffect, useState } from 'react';
import SystemHealthHeader from './components/SystemHealthHeader';
import SensorMatrix from './components/SensorMatrix';
import Charts from './components/Charts';
import TelemetryStream from './components/TelemetryStream';
import { WeatherData, SystemStatus, TelemetryLog } from './types';

const POLLING_INTERVAL = 4000;
const MAX_HISTORY = 50;
const MAX_LOGS = 100;

function App() {
  const [systemStatus, setSystemStatus] = useState<SystemStatus>({
    online: false,
    deviceId: 'ESP32-WS-001',
    lastSync: Date.now(),
    uptime: 0,
  });

  const [historicalData, setHistoricalData] = useState<WeatherData[]>([]);
  const [telemetryLogs, setTelemetryLogs] = useState<TelemetryLog[]>([]);
  const [startTime] = useState(Date.now());

  const calculateRateOfChange = (current: number, history: number[]): number => {
    if (history.length < 2) return 0;
    const oneHourAgo = history[Math.max(0, history.length - 12)];
    if (oneHourAgo === 0) return 0;
    return ((current - oneHourAgo) / oneHourAgo) * 100;
  };

  const fetchWeatherData = async () => {
    try {
      const response = await fetch('http://localhost:5000/api/live');
      const result = await response.json();

      if (result.status === 'online' && result.current) {
        const bd = result.current;

        const mapData = (h: any): WeatherData => ({
          temp_bme: h.temp_bme || h.temperature || 0,
          temp_dht: h.temp_dht || h.temperature || 0,
          hum_bme: h.hum_bme || h.humidity || 0,
          hum_dht: h.hum_dht || h.humidity || 0,
          pressure: h.pressure || 0,
          altitude: h.altitude || 0,
          light_level: h.light_level || h.light || 0,
          rain_level: h.rain_level || h.rain || 0,
          dew_point: h.dew_point || 0,
          heat_index: h.heat_index || 0,
          timestamp: h.timestamp * 1000,
          uptime: h.uptime || 0
        });

        const newData = mapData(bd);

        setHistoricalData((prev) => {
          if (result.history && prev.length === 0) {
            return result.history.map(mapData).slice(-MAX_HISTORY);
          }
          return [...prev, newData].slice(-MAX_HISTORY);
        });

        setTelemetryLogs((prev) => {
          const log: TelemetryLog = {
            id: `${Date.now()}-${Math.random()}`,
            timestamp: newData.timestamp,
            data: newData,
          };
          return [log, ...prev].slice(-MAX_LOGS);
        });

        setSystemStatus((prev) => ({
          ...prev,
          online: true,
          lastSync: Date.now(),
          uptime: newData.uptime || Math.floor((Date.now() - startTime) / 1000),
        }));
      } else {
        setSystemStatus((prev) => ({ ...prev, online: false }));
      }
    } catch (error) {
      console.error('Failed to fetch:', error);
      setSystemStatus((prev) => ({ ...prev, online: false }));
    }
  };

  useEffect(() => {
    fetchWeatherData();
    const interval = setInterval(fetchWeatherData, POLLING_INTERVAL);
    return () => clearInterval(interval);
  }, []);

  const current = historicalData[historicalData.length - 1];

  const metrics = {
    temp: {
      current: current?.temp_bme || 0,
      rateOfChange: calculateRateOfChange(current?.temp_bme || 0, historicalData.map(d => d.temp_bme)),
      unit: '°C'
    },
    hum: {
      current: current?.hum_bme || 0,
      rateOfChange: calculateRateOfChange(current?.hum_bme || 0, historicalData.map(d => d.hum_bme)),
      unit: '%'
    },
    pres: {
      current: current?.pressure || 0,
      rateOfChange: calculateRateOfChange(current?.pressure || 0, historicalData.map(d => d.pressure)),
      unit: 'hPa'
    },
    light: {
      current: current?.light_level || 0,
      rateOfChange: calculateRateOfChange(current?.light_level || 0, historicalData.map(d => d.light_level)),
      unit: '%'
    },
    rain: {
      current: current?.rain_level || 0,
      rateOfChange: calculateRateOfChange(current?.rain_level || 0, historicalData.map(d => d.rain_level)),
      unit: '%'
    }
  };

  return (
    <div className="min-h-screen bg-[#121212] text-white p-6">
      <div className="max-w-[1800px] mx-auto">
        <SystemHealthHeader status={systemStatus} />

        <SensorMatrix
          temperature={metrics.temp}
          humidity={metrics.hum}
          pressure={metrics.pres}
          light={metrics.light}
          rain={metrics.rain}
        />

        {/* New Comparison Row */}
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4 mb-8">
          <div className="bg-white/5 border border-white/10 p-4">
            <div className="text-[10px] text-white/40 uppercase font-mono mb-2">Secondary Sensor (DHT11)</div>
            <div className="flex justify-between items-center text-sm font-bold">
              <span>Temp: {current?.temp_dht.toFixed(1)}°C</span>
              <span>Hum: {current?.hum_dht.toFixed(1)}%</span>
            </div>
          </div>
          <div className="bg-white/5 border border-white/10 p-4">
            <div className="text-[10px] text-white/40 uppercase font-mono mb-2">Calculated Math</div>
            <div className="flex justify-between items-center text-sm font-bold">
              <span>DewPt: {current?.dew_point.toFixed(1)}°C</span>
              <span>HeatIdx: {current?.heat_index.toFixed(1)}°C</span>
            </div>
          </div>
          <div className="bg-white/5 border border-white/10 p-4">
            <div className="text-[10px] text-white/40 uppercase font-mono mb-2">Physics Data</div>
            <div className="text-sm font-bold">Altitude: {current?.altitude.toFixed(1)}m</div>
          </div>
          <div className="bg-white/5 border border-white/20 p-4 flex items-center justify-center">
            <div className="text-[10px] text-[#00FF88] uppercase font-bold tracking-tighter">Dual-Sensor Redundancy: Active</div>
          </div>
        </div>

        <Charts historicalData={historicalData} />

        <TelemetryStream logs={telemetryLogs} />
      </div>
    </div>
  );
}

export default App;

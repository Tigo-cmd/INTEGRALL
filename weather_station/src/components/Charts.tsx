import {
  LineChart,
  Line,
  BarChart,
  Bar,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
} from 'recharts';
import { WeatherData } from '../types';

interface ChartsProps {
  historicalData: WeatherData[];
}

export default function Charts({ historicalData }: ChartsProps) {
  const fmt = (ts: number) =>
    new Date(ts).toLocaleTimeString('en-US', { hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit' });

  // Chart 1: BME vs DHT Redundancy
  const comparisonData = historicalData.map((d) => ({
    time: fmt(d.timestamp),
    temp_bme: d.temp_bme,
    temp_dht: d.temp_dht,
    hum_bme: d.hum_bme,
    hum_dht: d.hum_dht,
  }));

  // Chart 2: Derived Math
  const physicsData = historicalData.map((d) => ({
    time: fmt(d.timestamp),
    dew_point: d.dew_point,
    heat_index: d.heat_index,
  }));

  // Chart 3: Light & Rain (last 15)
  const envData = historicalData.slice(-15).map((d) => ({
    time: fmt(d.timestamp),
    light: d.light_level,
    rain: d.rain_level,
  }));

  const tooltipStyle = {
    backgroundColor: '#1a1a1a',
    border: '1px solid #ffffff40',
    fontFamily: 'monospace',
    fontSize: '12px',
  };
  const legendStyle = { fontFamily: 'monospace', fontSize: '10px' };
  const axisStyle = { fontSize: '10px', fontFamily: 'monospace' };

  return (
    <div className="space-y-6 mb-8">
      {/* Row 1: Two charts side by side */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Sensor Redundancy */}
        <div className="bg-white/5 border-2 border-white/10 p-6">
          <h3 className="text-white font-mono text-sm uppercase tracking-widest mb-4">
            SENSOR REDUNDANCY (BME vs DHT)
          </h3>
          <ResponsiveContainer width="100%" height={300}>
            <LineChart data={comparisonData}>
              <CartesianGrid strokeDasharray="3 3" stroke="#ffffff20" />
              <XAxis dataKey="time" stroke="#ffffff60" style={axisStyle} />
              <YAxis stroke="#ffffff40" style={axisStyle} />
              <Tooltip contentStyle={tooltipStyle} />
              <Legend wrapperStyle={legendStyle} />
              <Line type="monotone" dataKey="temp_bme" stroke="#FF3E3E" strokeWidth={2} dot={false} name="Temp BME (°C)" />
              <Line type="monotone" dataKey="temp_dht" stroke="#FF8E8E" strokeWidth={1} strokeDasharray="5 5" dot={false} name="Temp DHT (°C)" />
              <Line type="monotone" dataKey="hum_bme" stroke="#00B8FF" strokeWidth={2} dot={false} name="Hum BME (%)" />
              <Line type="monotone" dataKey="hum_dht" stroke="#8EDFFF" strokeWidth={1} strokeDasharray="5 5" dot={false} name="Hum DHT (%)" />
            </LineChart>
          </ResponsiveContainer>
        </div>

        {/* Derived Metrics */}
        <div className="bg-white/5 border-2 border-white/10 p-6">
          <h3 className="text-white font-mono text-sm uppercase tracking-widest mb-4">
            DERIVED METRICS (DEW POINT & HEAT INDEX)
          </h3>
          <ResponsiveContainer width="100%" height={300}>
            <LineChart data={physicsData}>
              <CartesianGrid strokeDasharray="3 3" stroke="#ffffff20" />
              <XAxis dataKey="time" stroke="#ffffff60" style={axisStyle} />
              <YAxis stroke="#ffffff40" style={axisStyle} />
              <Tooltip contentStyle={tooltipStyle} />
              <Legend wrapperStyle={legendStyle} />
              <Line type="monotone" dataKey="dew_point" stroke="#D4FF33" strokeWidth={2} dot={false} name="Dew Point (°C)" />
              <Line type="monotone" dataKey="heat_index" stroke="#FFB800" strokeWidth={2} dot={false} name="Heat Index (°C)" />
            </LineChart>
          </ResponsiveContainer>
        </div>
      </div>

      {/* Row 2: Environment bar chart full width */}
      <div className="bg-white/5 border-2 border-white/10 p-6">
        <h3 className="text-white font-mono text-sm uppercase tracking-widest mb-4">
          LIGHT & RAIN INTENSITY (LAST 15 READINGS)
        </h3>
        <ResponsiveContainer width="100%" height={280}>
          <BarChart data={envData}>
            <CartesianGrid strokeDasharray="3 3" stroke="#ffffff20" />
            <XAxis dataKey="time" stroke="#ffffff60" style={axisStyle} />
            <YAxis stroke="#ffffff40" style={axisStyle} />
            <Tooltip contentStyle={tooltipStyle} />
            <Legend wrapperStyle={legendStyle} />
            <Bar dataKey="light" fill="#00FF88" name="Light (%)" />
            <Bar dataKey="rain" fill="#A855F7" name="Rain (%)" />
          </BarChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}

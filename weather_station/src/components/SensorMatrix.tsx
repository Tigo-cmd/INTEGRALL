import { Thermometer, Droplets, Gauge, Sun, CloudRain, TrendingUp, TrendingDown, Minus } from 'lucide-react';
import { SensorMetrics } from '../types';

interface SensorMatrixProps {
  temperature: SensorMetrics;
  humidity: SensorMetrics;
  pressure: SensorMetrics;
  light: SensorMetrics;
  rain: SensorMetrics;
}

interface SensorTileProps {
  label: string;
  icon: React.ReactNode;
  metrics: SensorMetrics;
  accentColor: string;
}

function SensorTile({ label, icon, metrics, accentColor }: SensorTileProps) {
  const getTrendIcon = () => {
    if (metrics.rateOfChange > 0.1) return <TrendingUp className="w-4 h-4" />;
    if (metrics.rateOfChange < -0.1) return <TrendingDown className="w-4 h-4" />;
    return <Minus className="w-4 h-4" />;
  };

  const getTrendColor = () => {
    if (metrics.rateOfChange > 0.1) return 'text-[#00FF88]';
    if (metrics.rateOfChange < -0.1) return 'text-[#FF3E3E]';
    return 'text-white/40';
  };

  return (
    <div className="bg-white/5 border-2 border-white/10 p-6 hover:border-white/20 transition-colors">
      <div className="flex items-start justify-between mb-4">
        <div className="flex items-center gap-3">
          <div className={accentColor}>
            {icon}
          </div>
          <div className="text-white/60 font-mono text-xs uppercase tracking-widest">{label}</div>
        </div>
        <div className={`flex items-center gap-1 ${getTrendColor()}`}>
          {getTrendIcon()}
        </div>
      </div>

      <div className="space-y-2">
        <div className="text-5xl font-bold text-white tabular-nums tracking-tight">
          {metrics.current.toFixed(1)}
        </div>
        <div className="text-white/40 font-mono text-sm">{metrics.unit}</div>
        <div className={`font-mono text-xs ${getTrendColor()}`}>
          {metrics.rateOfChange > 0 ? '+' : ''}{metrics.rateOfChange.toFixed(2)}% in last 1h
        </div>
      </div>
    </div>
  );
}

export default function SensorMatrix({ temperature, humidity, pressure, light, rain }: SensorMatrixProps) {
  return (
    <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-5 gap-4 mb-8">
      <SensorTile
        label="TEMPERATURE"
        icon={<Thermometer className="w-6 h-6" strokeWidth={2.5} />}
        metrics={temperature}
        accentColor="text-[#FF3E3E]"
      />
      <SensorTile
        label="HUMIDITY"
        icon={<Droplets className="w-6 h-6" strokeWidth={2.5} />}
        metrics={humidity}
        accentColor="text-[#00B8FF]"
      />
      <SensorTile
        label="PRESSURE"
        icon={<Gauge className="w-6 h-6" strokeWidth={2.5} />}
        metrics={pressure}
        accentColor="text-[#FFB800]"
      />
      <SensorTile
        label="LIGHT"
        icon={<Sun className="w-6 h-6" strokeWidth={2.5} />}
        metrics={light}
        accentColor="text-[#00FF88]"
      />
      <SensorTile
        label="RAIN"
        icon={<CloudRain className="w-6 h-6" strokeWidth={2.5} />}
        metrics={rain}
        accentColor="text-[#A855F7]"
      />
    </div>
  );
}

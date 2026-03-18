export interface WeatherData {
  // Redundant data
  temp_bme: number;
  temp_dht: number;
  hum_bme: number;
  hum_dht: number;

  // Environment
  pressure: number;
  altitude: number;
  light_level: number;
  rain_level: number;

  // Calculated Math
  dew_point: number;
  heat_index: number;

  timestamp: number;
  uptime?: number;
}

export interface SystemStatus {
  online: boolean;
  deviceId: string;
  lastSync: number;
  uptime: number;
}

export interface SensorMetrics {
  current: number;
  rateOfChange: number;
  unit: string;
}

export interface TelemetryLog {
  id: string;
  timestamp: number;
  data: WeatherData;
}

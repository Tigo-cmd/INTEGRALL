import requests
import time
import random
import math

BACKEND_URL = "http://localhost:5000/telemetry"

def simulate_esp32():
    print(f"ESP32 Simulator Started → {BACKEND_URL}")
    print("Sending: temp_bme, temp_dht, hum_bme, hum_dht, pressure, altitude, dew_point, heat_index, light_level, rain_level")
    print("-" * 60)
    uptime = 0
    base_temp = 22.0
    
    try:
        while True:
            # Simulate realistic sensor drift
            variation = math.sin(uptime / 50) * 1.5
            
            temp_bme = round(base_temp + variation + random.uniform(-0.3, 0.3), 2)
            hum_bme = round(55 + variation * 2 + random.uniform(-1, 1), 1)
            pressure = round(1013.25 + variation * 0.5 + random.uniform(-0.2, 0.2), 2)
            
            # DHT is slightly less accurate than BME (realistic)
            temp_dht = round(temp_bme + random.uniform(-0.8, 0.8), 2)
            hum_dht = round(hum_bme + random.uniform(-3, 3), 1)
            
            # Derived math
            altitude = round(44330 * (1.0 - (pressure / 1013.25) ** 0.1903), 1)
            dew_point = round(temp_bme - ((100 - hum_bme) / 5), 2)
            heat_index = round(temp_bme + 0.33 * hum_bme - 4, 2)
            
            # Environment
            light_level = random.randint(20, 95)
            rain_level = random.randint(0, 15) if random.random() > 0.3 else random.randint(40, 90)

            data = {
                "temp_bme": temp_bme,
                "temp_dht": temp_dht,
                "hum_bme": hum_bme,
                "hum_dht": hum_dht,
                "pressure": pressure,
                "altitude": altitude,
                "dew_point": dew_point,
                "heat_index": heat_index,
                "light_level": light_level,
                "rain_level": rain_level,
                "uptime": uptime
            }
            
            try:
                r = requests.post(BACKEND_URL, json=data)
                status = "OK" if r.status_code == 200 else f"ERR:{r.status_code}"
                print(f"[{status}] T_BME:{temp_bme} T_DHT:{temp_dht} H:{hum_bme} P:{pressure} DewPt:{dew_point} Rain:{rain_level}%")
            except Exception as e:
                print(f"[FAIL] {e}")
            
            uptime += 5
            time.sleep(5)
    except KeyboardInterrupt:
        print("\nSimulator stopped.")

if __name__ == "__main__":
    simulate_esp32()
